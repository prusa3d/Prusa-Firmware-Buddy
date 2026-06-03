/// @file
#pragma once

#include "partial_file.hpp"
#include "cmsis_os.h"
#include <atomic>
#include <array>
#include "usbh_async_diskio.hpp"
#include <printers.h>
#include <common/unique_file_ptr.hpp>
#include <freertos/counting_semaphore.hpp>

namespace transfers {

/// USB/FatFS partial file implementation using direct sector writes.
///
/// The file is always contiguous on the drive (preallocated on create). This
/// makes a lot of things easier, but we won't be able to use all the space on
/// the drive if it's fragmented.
class UsbPartialFile : public PartialFile {
public:
#if PRINTER_IS_PRUSA_MINI()
    static constexpr size_t SECTORS_PER_WRITE = 1; // Low on RAM on mini
#else
    static constexpr size_t SECTORS_PER_WRITE = 8;
#endif
    /// The size of one write buffer (possibly multiple sectors)
    static constexpr size_t BUFFER_SIZE = SECTOR_SIZE * SECTORS_PER_WRITE;

    UsbPartialFile(UsbhMscRequest::LunNbr drive, UsbhMscRequest::SectorNbr first_sector, State state, int file_lock);
    ~UsbPartialFile() override;

    /// Construct from a file already opened by the PartialFile factory.
    static Result create(const char *path, unique_file_ptr file, size_t size);
    static Result open(const char *path, unique_file_ptr file, State state);

    bool supports_random_write() const override { return true; }
    bool seek(size_t offset) override;
    size_t tell() override { return current_offset; }
    bool write(const uint8_t *data, size_t size) override;
    bool sync() override;
    void release_file() override;

    BufferPeek get_current_buffer(bool blocking_wait) override;
    bool advance_written(size_t by) override;

private:
    static void usb_msc_write_finished_callback(USBH_StatusTypeDef result, void *param1, void *param2);

    /// Pre-allocated request pool for the usbh_msc_submit_request operation
    /// with dynamically allocated memory for the sectors (needs DMA, so not
    /// on a stack that can be put into CCMRAM).
    struct SectorPool {
        /// sync operations require a minimum of 2 slots and 32 is the maximum
        /// due to slot_mask
        static constexpr uint32_t size = 2;
        static_assert(size >= 2 && size <= 32);

        SectorPool(UsbhMscRequest::LunNbr lun, UsbhMscRequestCallback callback, void *callback_param1);
        ~SectorPool();

        /// Get a free slot; if none is available, waits until one becomes
        /// free (returns nullptr in case of timeout).
        UsbhMscRequest *acquire(bool block_waiting);

        /// Release a previously acquired slot.
        void release(uint32_t slot);

        /// Blocks until all slots (except the `avoid` skipped ones) are
        /// released. If force is true, waits potentially forever for the
        /// operations to complete (to be used in the destructor, so under no
        /// circumstances a reference-after-free may happen).
        bool sync(uint32_t avoid, bool force);

    private:
        bool is_available_slot() const { return slot_mask != ~0u; }
        uint32_t get_available_slot() const;

        /// Protects the slots acquisition / mask
        freertos::Mutex mutex;
        /// Represents the number of free slots
        freertos::CountingSemaphore semaphore;
        /// One bit per slot from the least significant (1 - acquired/unused, 0 - free)
        uint32_t slot_mask;
        UsbhMscRequest pool[size];
    };

    SectorPool sector_pool;

    /// Extend the valid parts by these once the relevant sectors are written
    /// (indexed by the pool slot)
    std::array<ValidPart, SectorPool::size> future_extend;

    /// Asynchronous write operation completed callback
    void usbh_msc_finished(USBH_StatusTypeDef result, uint32_t slot);

    /// Whether an error occurred during writing (set asynchronously from the callback)
    std::atomic<bool> write_error;

    /// USB sector number where the first data of the file are located
    UsbhMscRequest::SectorNbr first_sector_nbr;

    /// Write buffer for the active sector the user is writing to
    UsbhMscRequest *current_sector;

    /// Offset ("ftell") within the file where the user will write next
    size_t current_offset;

    /// Translate file offset to sector number
    UsbhMscRequest::SectorNbr get_sector_nbr(size_t offset);

    /// Translate sector number to file offset
    size_t get_offset(UsbhMscRequest::SectorNbr sector_nbr);

    /// Write current sector over USB to the FatFS drive
    bool write_current_sector();

    /// Discard the current sector, releasing it back to the sector_pool
    void discard_current_sector();

    /// Read-only fd kept open for our lifetime to lock the file in place so
    /// somebody doesn't accidentally delete it or mess with it in a different
    /// way. (An fd instead of FILE * because it's more lightweight and we
    /// don't actually _use_ it for anything.)
    int file_lock;

    /// How large (in bytes) write are we going to do now?
    ///
    /// Depends on if the current sector is "aligned" to the write size or not.
    ///
    /// Current buffer must be allocated at this point.
    size_t allowed_write_size() const;

    /// How many sectors can we write (either aligned completely or to round
    /// up to alignment for next write).
    size_t allowed_sectors() const;
};

} // namespace transfers
