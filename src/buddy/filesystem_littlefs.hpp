#pragma once

#include <option/has_internal_storage_flash.h>

#include <cstdint>
#include <memory>

/// Initialize all persistent littlefs filesystems (NOR system, NAND internal).
void filesystem_littlefs_init();

#if HAS_INTERNAL_STORAGE_FLASH()
/// Unmount the internal storage filesystem, e.g. before erasing the chip.
void internal_storage_unmount();

/// Whether the internal storage chip responded at boot. A prerequisite for
/// the destructive test - never erase or program an unidentified chip.
bool internal_storage_initialized();

/// Erase the NAND storage chip, reformat the littlefs filesystem, and remount it.
/// Returns true on success. Callers must ensure no other task is using the
/// storage filesystem during the operation.
bool internal_storage_format();

/// Non-destructive consistency check: traverses the internal storage filesystem
/// to surface filesystem or flash errors. Returns true if no errors were detected.
bool internal_storage_check();

/// Destructive read/write test of the entire internal storage chip, driven one
/// block at a time so callers can pump the GUI between iterations.
///
/// Construction unmounts the storage filesystem and heap-allocates two page
/// buffers. Repeatedly call step() until it returns true; the final step
/// reformats and remounts the filesystem. Bad blocks (erase/program/read
/// errors or data mismatches) are logged with per-failure details and counted
/// in bad_blocks().
///
/// Callers must ensure no other task is using the storage filesystem.
class InternalStorageTester {
public:
    enum class Phase {
        testing,
        restoring,
        done,
    };

    InternalStorageTester();

    /// Process one increment of the test. Returns true when the entire process
    /// (including reformat + remount) is complete.
    bool step();

    Phase phase() const { return current_phase; }
    uint32_t current_block() const { return current; }
    uint32_t total_blocks() const { return total; }
    uint32_t bad_blocks() const { return bad; }
    bool format_succeeded() const { return format_ok; }

private:
    void test_block(uint32_t block);

    Phase current_phase = Phase::testing;
    uint32_t current = 0;
    uint32_t total;
    uint32_t bad = 0;
    bool format_ok = true;
    std::unique_ptr<uint8_t[]> wbuf;
    std::unique_ptr<uint8_t[]> rbuf;
};
#endif
