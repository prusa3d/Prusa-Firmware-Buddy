#pragma once
#include "cmsis_os.h"
#include <memory>
#include <atomic>
#include <variant>
#include <optional>
#include <array>
#include <stdint.h>
#include "usbh_async_diskio.hpp"
#include <printers.h>
#include <common/unique_file_ptr.hpp>
#include <freertos/mutex.hpp>
#include <freertos/counting_semaphore.hpp>

namespace transfers {

/// Partial File manages a FatFS file that can be read & written at the same time.
///
/// - The file is always contiguous on the drive.
///     - This makes a lot of things easier, but we won't be able to use all the space on the drive if it's fragmented.
/// - The file, once created, can be read by standard means (fread etc)
///     - To observe which parts of the file are valid for reading, use the `get_valid_head` and `get_valid_tail` methods.
/// - To write to the file, use the `write`, `seek` and `flush` methods.
///     - The file remembers which parts of the file are valid. But there are limitations in order to keep the implementation simple.
///     - It remembers up to 2 valid independent parts. No more.
///     - One of them is called the "head", which is a part starting at offset 0.
///     - Second one is called the "tail" and it's a part starting somewhere in the middle of the
///         file (gradually growing to the end of the file).
///     - Creating a third valid part (by writing somewhere in between the head and the tail, for example) is not allowed.
///     - Therefore, every write should either extend the head or the tail.
///     - At some point, when the head and the tail meet, they are merged (tail is extended to the start of the file
///         and head is extended to the end of the file).
///     - Writing uses low-level USB functions and has basic buffering implemented by this class. Some requirements:
///         - seek() is allowed only to the start of a sector
///         - consecutive writes gradually fill the sector
///         - when a sector is fully written to, it's flushed to the drive
///         - seek()  to a different sector while the current one hasn't been fully written to will discard the currently buffered data
///
/// Thread safety:
/// - Getting the state (and related methods) are thread safe - it can happen concurrently to writes from other thread.
class PartialFile {
public:
    static const size_t SECTOR_SIZE = 512;
#if PRINTER_IS_PRUSA_MINI()
    static const size_t SECTORS_PER_WRITE = 1; // Low on RAM on mini
#else
    static const size_t SECTORS_PER_WRITE = 8;
#endif
    // The size of one buffer (possibly multiple sectors)
    static const size_t BUFFER_SIZE = SECTOR_SIZE * SECTORS_PER_WRITE;

    struct ValidPart {
        size_t start;
        size_t end; // exclusive

        void merge(const ValidPart &other) {
            // this:  oooox
            // other:     oooox
            if (other.start <= end && other.end > end) {
                // extend to the right
                end = other.end;
            }
            // this:        oooox
            // other:   oooox
            // other:      ox
            if (other.start < start && other.end >= start) {
                // extend to the left
                start = other.start;
            }
        }
    };

    struct State {
        std::optional<ValidPart> valid_head;
        std::optional<ValidPart> valid_tail;
        size_t total_size;

        size_t get_valid_size() const {
            size_t bytes_head = valid_head.has_value() ? valid_head->end - valid_head->start : 0;
            size_t bytes_tail = valid_tail.has_value() ? valid_tail->end - valid_tail->start : 0;
            size_t bytes_overlap = 0;
            if (valid_head.has_value() && valid_tail.has_value() && valid_head->end > valid_tail->start) {
                bytes_overlap = valid_head->end - valid_tail->start;
            }
            return bytes_head + bytes_tail - bytes_overlap;
        }

        size_t get_percent_valid() const {
            // Needs to be calculated in float because (100 * size) overflows size_t
            return total_size ? static_cast<float>(get_valid_size()) * 100.0f / total_size : 0;
        }

        void extend_head(size_t bytes) {
            if (valid_head.has_value()) {
                valid_head->end += bytes;
            } else {
                valid_head = { 0, bytes };
            }
        }
    };

private:
    static void usb_msc_write_finished_callback(USBH_StatusTypeDef result, void *param1, void *param2);

    /// Pre-allocated request pool for usbh_msc_submit_request operation with dynamically
    /// allocated memory for sectors (needs DMA so not on stack that can be put into CCMRAM)
    struct SectorPool {
        /// sync operations require a minimum of 2 slots and 32 is the maximum due to slot_mask
        static constexpr uint32_t size = 2;
        static_assert(size >= 2 && size <= 32);

        SectorPool(UsbhMscRequest::LunNbr lun, UsbhMscRequestCallback callback, void *callback_param1);
        ~SectorPool();

        /// Get a free slot, if none is available, it waits until it becomes free (returns nullptr in case of timeout)
        ///
        /// Returns the slot and its index (can be paired with the number given to the callback)
        UsbhMscRequest *acquire(bool block_waiting);

        /// Release a previously acquired slot
        void release(uint32_t slot);

        /// Blocks until all slots (except skipped ones) are relesed
        /// If force is true, it waits potentially forever for the operations to complete
        /// (to be used in destructor, so under no circumstances a reference-after-free may happen)
        bool sync(uint32_t avoid, bool force);

    private:
        bool is_available_slot() const { return slot_mask != ~0u; }

        uint32_t get_available_slot() const;

        // Protects the slots acquisition / mask
        freertos::Mutex mutex;
        // Represents the number of free slots (update of mask must be protected by this)
        freertos::CountingSemaphore semaphore;

        // Mask of acquired/free slots one bit per slot from least significant (1-acquired/unused, 0-free)
        uint32_t slot_mask;

        UsbhMscRequest pool[size];
    };

    // Pre-allocated request pool of sectors
    SectorPool sector_pool;

    // Extend the valid parts by these once the relevant sectors are written
    // (indexed by the sector number)
    std::array<ValidPart, SectorPool::size> future_extend;

    // Asynchronous write operation completed callback
    void usbh_msc_finished(USBH_StatusTypeDef result, uint32_t slot);

    /// Flag whether an error occurred during writing (set asynchronously from the callback)
    std::atomic<bool> write_error;

    /// USB sector number where the first data of the file are located
    UsbhMscRequest::SectorNbr first_sector_nbr;

    /// Write buffer for the active sector the user is writing to
    UsbhMscRequest *current_sector;

    /// Offset ("ftell") within the file where the user will write next
    size_t current_offset;

    /// Valid parts of the file
    State state;

    mutable freertos::Mutex state_mutex;

    /// Last reported progress over logs
    int last_progress_percent;

    /// Translate file offset to sector number
    UsbhMscRequest::SectorNbr get_sector_nbr(size_t offset);

    /// Translate sector number to file offset
    size_t get_offset(UsbhMscRequest::SectorNbr sector_nbr);

    /// Write current sector over USB to the FatFS drive
    bool write_current_sector();

    /// Discard current sector - it is necessary to release it from the sector_pool
    void discard_current_sector();

    /// Extend the valid_head and/or valid_tail to include the new_part
    void extend_valid_part(ValidPart new_part);

    /// Keeping a read-only open file.
    ///
    /// This is to lock the file in place so somebody doesn't accidentally
    /// delete it or mess with it in a different way.
    ///
    /// (Using fd instead of FILE * here because it's more lightweight and we
    /// don't actually _use_ it for anything).
    int file_lock;

    typedef void WrittenCallback(void *);
    WrittenCallback *written_callback = nullptr;
    void *written_callback_arg;

    /// How large (in bytes) write are we going to do now?
    ///
    /// Depends on if the current sector is "aligned" to the write size or not.
    ///
    /// Current buffer must be allocated at this point.
    size_t allowed_write_size() const;

    /// How many sectors can we write (either aligned completely or to round up
    /// to alignment for next write).
    size_t allowed_sectors() const;

public:
    PartialFile(UsbhMscRequest::LunNbr drive, UsbhMscRequest::SectorNbr first_sector, State state, int file_lock);
    ~PartialFile();
    using Ptr = std::shared_ptr<PartialFile>;
    using Result = std::variant<const char *, PartialFile::Ptr>;

    /// Try to create a new partial file of preallocated size
    static Result create(const char *path, size_t size);

    /// Open existing partial file
    ///
    /// state.total_size is updated according to what is found on the disk and overwritten.
    ///
    /// * ignore_opened: If set to true, it'll open the file (for writing) even
    ///   if there's a reader somewhere else.
    static Result open(const char *path, State state, bool ignore_opened);

    /// Convert an open FILE * into this.
    ///
    /// state.total_size is updated according to what is found on the disk and overwritten.
    static Result convert(const char *path, unique_file_ptr file, State state);

    /// Seek to a given offset within the file
    bool seek(size_t offset);

    /// Position in the file
    size_t tell() {
        return current_offset;
    }

    struct WouldBlock {};
    struct WriteError {};
    struct OutOfRange {};

    struct BufferAndSizes {
        uint8_t *buffer;
        // Already written into the buffer this far
        size_t offset;
        // Usable size of the buffer.
        //
        // Note: may be different each time (sometimes we allow "big" writes,
        // sometimes small ones).
        //
        // This does not account for "end of file", that's still up to the
        // caller.
        size_t size;
    };
    using BufferPeek = std::variant<BufferAndSizes, WouldBlock, WriteError, OutOfRange>;

    /// Provides a data pointer and offset (to the place where already written) to the current write buffer.
    ///
    /// The caller is responsible to offset the place where it writes.
    ///
    /// Will allocate a new one as needed. If blocking_wait is set to true and
    /// no buffer is available for allocation, it waits for one to become
    /// available.
    ///
    /// The total buffer is always SECTOR_SIZE large.
    BufferPeek get_current_buffer(bool blocking_wait);

    /// Advance the write position, submit the current buffer to USB if completely full.
    bool advance_written(size_t by);

    /// Write data to the file at current offset
    bool write(const uint8_t *data, size_t size);

    /// Flush the current sector to the USB drive
    bool sync();

    /// "Close" the file while still preserving the state and size.
    ///
    /// No further writes will succeed or be made and the file lock is released.
    void release_file();

    /// Get the final size of the file
    size_t final_size() const { return get_state().total_size; }

    /// Get the valid part of the file starting at offset 0
    std::optional<ValidPart> get_valid_head() const { return get_state().valid_head; }

    /// Get the valid part of the file starting passed the head
    std::optional<ValidPart> get_valid_tail() const { return get_state().valid_tail; }

    /// Check if the file has valid data at [0, bytes) range
    bool has_valid_head(size_t bytes) const;

    /// Check if the file has valid data at [file_size - bytes, file_size) range
    bool has_valid_tail(size_t bytes) const;

    State get_state() const;

    void print_progress();

    // Callback to be called whenever a block is written/failed to be written.
    //
    // Can be set to null.
    void set_written_callback(WrittenCallback *cback, void *arg) {
        written_callback = cback;
        written_callback_arg = arg;
    }
};

} // namespace transfers
