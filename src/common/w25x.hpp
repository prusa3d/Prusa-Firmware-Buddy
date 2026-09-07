/// @file
/// Driver for the W25xxx family of SPI NOR flash memories.
///
/// SPI bus communication is handled by SpiFlashBus (spi_flash_bus.hpp).
#pragma once

#include <common/Pin.hpp>
#include <cstdint>
#include <device/board.h>
#include <freertos/mutex.hpp>

class SpiFlashBus;

class W25xFlash {
public:
    /// Smallest erasable unit. Called "sector" in W25Q NOR flash terminology,
    /// but we use "block" to match littlefs and NAND conventions.
    static constexpr uint32_t block_size = 4096;
    static constexpr uint32_t block64_size = 0x10000;

    // Address layout constants
    static constexpr uint32_t dump_start_address = 0;
#if BOARD_IS_BUDDY()
    // Some MINIes have 1MB flash, some have 8M
    // 49 = 196KiB offset for crash dump
    static constexpr uint32_t error_start_address = 49 * block_size;
    static constexpr uint32_t pp_start_address = 50 * block_size;
    static constexpr uint32_t fs_start_address = 51 * block_size;
#elif BOARD_IS_XBUDDY()
    // 8M = 2K of 4K blocks
    // 65 = 260KiB offset for crash dump
    static constexpr uint32_t error_start_address = 65 * block_size;
    static constexpr uint32_t pp_start_address = 66 * block_size;
    static constexpr uint32_t fs_start_address = 67 * block_size;
#elif BOARD_IS_XLBUDDY()
    // 8M = 2K of 4K blocks
    // 65 = 260KiB offset for crash dump, which is the total RAM size
    static constexpr uint32_t error_start_address = 65 * block_size;
    static constexpr uint32_t pp_start_address = 66 * block_size;
    static constexpr uint32_t fs_start_address = 68 * block_size;
#else
    #error "Unsupported board type"
#endif

    static constexpr uint32_t pp_size = fs_start_address - pp_start_address;
    static constexpr uint32_t dump_size = error_start_address - dump_start_address;

    static W25xFlash &instance();

    /// Initialize while the scheduler is running.
    ///
    /// When initialized using this function, all interface functions
    /// must be called in task context only.
    bool init();

    /// (Re)initialize after the scheduler has been stopped.
    ///
    /// Call this once to abort ongoing transfers and take over the chip.
    /// This should only be used to write crash dump to external flash.
    ///
    /// All interface functions can be called in any context but are not reentrant.
    /// If reinitialized during DMA transfer it is aborted. If some
    /// data is already transfered to the chip at that point those data are
    /// written gracefully. If erase operation is ongoing it is completed
    /// during reinitialization.
    ///
    /// Worst case runtime is 100 seconds if called just after chip erase
    /// operation has been started. Worst case runtime is 200 seconds for
    /// maliciously crafted w25x responses.
    ///
    /// @retval true on success
    /// @retval false otherwise.
    bool reinit_before_crash_dump();

    /// Read data from the flash.
    /// Errors can be checked (and cleared) using fetch_error()
    void read(uint32_t addr, uint8_t *data, uint32_t len);

    /// Write data to the flash (the sector has to be erased first).
    /// Errors can be checked (and cleared) using fetch_error()
    void program(uint32_t addr, const uint8_t *data, uint32_t len);

    /// Erase single sector (4KB) of the flash.
    /// Errors can be checked (and cleared) using fetch_error()
    void erase_block(uint32_t addr);

    /// Erase block of 32KB of the flash.
    /// Errors can be checked (and cleared) using fetch_error()
    void block32_erase(uint32_t addr);

    /// Erase block of 64KB of the flash.
    /// Errors can be checked (and cleared) using fetch_error()
    void block64_erase(uint32_t addr);

    uint32_t erase_block_size() const { return block_size; }

    /// Return the number of available sectors.
    uint32_t block_count() const;

    uint32_t capacity() const { return erase_block_size() * block_count(); }

    /// Fetch and clear error of a previous operation.
    /// Returns 0 if there hasn't been any error.
    int fetch_error();

private:
    SpiFlashBus &bus;
    const buddy::hw::OutputPin &cs;

    /// Lock ordering: erase_mutex must be acquired before bus.lock().
    ///
    /// Erase operations:
    ///   erase_mutex acquired, then bus locked.
    ///   is_erasing set to true, start erase command.
    ///   Bus unlocked (bus free while chip erases internally).
    ///   Bus re-locked per status poll iteration.
    ///   When done, is_erasing set to false.
    ///   Bus unlocked, then erase_mutex released.
    ///
    /// Read/write (standard priority):
    ///   erase_mutex acquired, then bus locked.
    ///   Do read/write operation.
    ///   Bus unlocked, then erase_mutex released.
    ///
    /// Write (high priority, power panic address range):
    ///   Try to acquire erase_mutex (non-blocking).
    ///   Lock bus, check is_erasing.
    ///   If is_erasing, suspend erase.
    ///   Do write operation.
    ///   If is_erasing, resume erase.
    ///   Unlock bus, then release erase_mutex (if acquired).
    ///
    /// All locking becomes no-op when bus.is_scheduler_stopped() is true
    /// (crash dump mode).
    freertos::Mutex erase_mutex;

    /// Block erase operation is in progress.
    ///
    /// Can be modified only when both erase_mutex and bus mutex are held.
    /// Can be read only when bus mutex is held.
    ///
    /// Only needs to be checked when erase_mutex was not acquired
    /// successfully; holding erase_mutex is sufficient to know erasing
    /// is not in progress.
    ///
    /// This exists because both mutexes must be held to start an erase.
    /// The erase implementation sets this to true once it acquires both.
    /// This ensures a high priority write doesn't assume erase is already
    /// ongoing if it fails to acquire erase_mutex and acquires the bus
    /// mutex in the moment erase acquired erase_mutex but didn't acquire
    /// the bus mutex yet.
    bool is_erasing = false;

    uint8_t device_id = 0;
    int current_error = 0;

    W25xFlash(SpiFlashBus &bus, const buddy::hw::OutputPin &cs);

    void unlock_bus();
    void lock_erase();
    void unlock_erase();
    [[nodiscard]] bool try_lock_erase();

    void select();
    void deselect();
    void write_enable();
    uint8_t read_status1();
    uint8_t read_status2();
    bool is_suspended();
    bool wait_busy();
    bool wait_erase();
    void erase(uint8_t cmd, uint32_t addr);
    bool check_id(uint8_t *out_devid);
    bool init_internal();

    void program_page(uint32_t addr, const uint8_t *data, uint16_t cnt, bool high_priority);
    void split_page_program(uint32_t addr, const uint8_t *data, uint32_t cnt, bool high_priority);
    void suspend_erase();
    void resume_erase();

    /// Set error (sticky — only stores the first error until cleared).
    void set_error(int error);
};
