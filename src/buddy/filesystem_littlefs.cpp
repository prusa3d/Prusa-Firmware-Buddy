#include "filesystem_littlefs.hpp"
#include "filesystem_littlefs_device.hpp"
#include "littlefs_flash_device.hpp"

#include <logging/log.hpp>
#include <buddy/filename_defs.hpp>
#include <common/w25x.hpp>
#if HAS_INTERNAL_STORAGE_FLASH()
    #include <common/mt29f_flash.hpp>
#endif

DEFINE_LITTLEFS_DEVICE(system)

#if HAS_INTERNAL_STORAGE_FLASH()
DEFINE_LITTLEFS_DEVICE(internal)

static LittlefsFlashDevice<Mt29fFlash> &internal_storage_device_instance() {
    // Cap filenames on the MT29F filesystem. Not applied to the W25x /system
    // filesystem below, which was formatted with the littlefs default name_max;
    // lowering it would fail the mount and trigger a reformat.
    static LittlefsFlashDevice<Mt29fFlash> instance(Mt29fFlash::instance(), 0, filename_defs::max_filename_length);
    return instance;
}

static bool chip_erase_and_mount() {
    Mt29fFlash::instance().chip_erase();
    if (int err = Mt29fFlash::instance().fetch_error()) {
        log_error(FileSystem, "MT29F chip erase failed: %d", err);
        return false;
    }
    // init() will fail to mount the erased chip, format it, and mount the fresh filesystem
    lfs_t *lfs = internal_storage_device_instance().init();
    if (!lfs) {
        log_error(FileSystem, "Internal storage mount failed");
        return false;
    }
    internal_ns::mount(lfs);
    return true;
}
#endif

void filesystem_littlefs_init() {
    static LittlefsFlashDevice<W25xFlash> system_device(W25xFlash::instance(), W25xFlash::fs_start_address);
    if (lfs_t *lfs = system_device.init()) {
        system_ns::mount(lfs);
    }

#if HAS_INTERNAL_STORAGE_FLASH()
    if (!Mt29fFlash::instance().is_initialized()) {
        // Don't mount or erase an unresponsive or misidentified chip
        return;
    }
    // Retry mounting to rule out a transient IO error: the recovery below is
    // destructive and must not trigger on a bus hiccup
    lfs_t *lfs = nullptr;
    for (int attempt = 0; attempt < 3 && !lfs; attempt++) {
        lfs = internal_storage_device_instance().init();
    }
    if (lfs) {
        internal_ns::mount(lfs);
    } else {
        // First boot (factory pages have invalid ECC) or corrupted beyond
        // mounting, chip erase to get a valid ECC state
        chip_erase_and_mount();
    }
#endif
}

#if HAS_INTERNAL_STORAGE_FLASH()
void internal_storage_unmount() {
    internal_ns::unmount();
}

bool internal_storage_initialized() {
    return Mt29fFlash::instance().is_initialized();
}

bool internal_storage_format() {
    if (!Mt29fFlash::instance().is_initialized()) {
        return false;
    }
    internal_storage_unmount();
    return chip_erase_and_mount();
}

bool internal_storage_check() {
    if (!internal_ns::get_lfs()) {
        log_error(FileSystem, "Internal storage not mounted");
        return false;
    }
    (void)Mt29fFlash::instance().fetch_error(); // clear any stale state before the check

    auto traverse_cb = [](void *, lfs_block_t) -> int { return 0; };
    if (int err = lfs_fs_traverse(internal_ns::get_lfs(), traverse_cb, nullptr); err < 0) {
        log_error(FileSystem, "Internal storage traverse failed: %d", err);
        return false;
    }
    if (int err = Mt29fFlash::instance().fetch_error()) {
        log_error(FileSystem, "Internal storage flash error during check: %d", err);
        return false;
    }
    return true;
}

// Pattern derived from absolute byte address — every cell holds a distinct value
// within a 32-bit window, so misrouted writes and stuck cells both surface.
static uint8_t expected_byte(uint32_t addr) {
    return static_cast<uint8_t>(addr ^ (addr >> 8) ^ (addr >> 16) ^ (addr >> 24));
}

InternalStorageTester::InternalStorageTester()
    : total(Mt29fFlash::instance().block_count())
    , wbuf(std::make_unique<uint8_t[]>(Mt29fFlash::page_size))
    , rbuf(std::make_unique<uint8_t[]>(Mt29fFlash::page_size)) {
    internal_storage_unmount();
    (void)Mt29fFlash::instance().fetch_error(); // clear any stale state
}

void InternalStorageTester::test_block(uint32_t block) {
    auto &nand = Mt29fFlash::instance();
    constexpr uint32_t block_sz = Mt29fFlash::block_size;
    constexpr uint32_t page_sz = Mt29fFlash::page_size;
    constexpr uint32_t pages_per_block = Mt29fFlash::pages_per_block;

    const uint32_t base = block * block_sz;

    nand.erase_block(base);
    if (int err = nand.fetch_error()) {
        log_error(FileSystem, "Block %lu erase failed: %d", static_cast<unsigned long>(block), err);
        bad++;
        return;
    }

    for (uint32_t p = 0; p < pages_per_block; p++) {
        const uint32_t addr = base + p * page_sz;
        for (uint32_t i = 0; i < page_sz; i++) {
            wbuf[i] = expected_byte(addr + i);
        }
        nand.program(addr, wbuf.get(), page_sz);
        if (int err = nand.fetch_error()) {
            log_error(FileSystem, "Block %lu page %lu program failed: %d",
                static_cast<unsigned long>(block), static_cast<unsigned long>(p), err);
            bad++;
            return;
        }
    }

    for (uint32_t p = 0; p < pages_per_block; p++) {
        const uint32_t addr = base + p * page_sz;
        nand.read(addr, rbuf.get(), page_sz);
        if (int err = nand.fetch_error()) {
            log_error(FileSystem, "Block %lu page %lu read failed: %d",
                static_cast<unsigned long>(block), static_cast<unsigned long>(p), err);
            bad++;
            return;
        }
        for (uint32_t i = 0; i < page_sz; i++) {
            if (rbuf[i] != expected_byte(addr + i)) {
                log_error(FileSystem, "Block %lu page %lu offset %lu: expected 0x%02x got 0x%02x",
                    static_cast<unsigned long>(block), static_cast<unsigned long>(p),
                    static_cast<unsigned long>(i), expected_byte(addr + i), rbuf[i]);
                bad++;
                return;
            }
        }
    }
}

bool InternalStorageTester::step() {
    switch (current_phase) {
    case Phase::testing:
        test_block(current++);
        if (current >= total) {
            current_phase = Phase::restoring;
        }
        return false;

    case Phase::restoring:
        if (bad > 0) {
            log_error(FileSystem, "Internal storage test: %lu bad blocks out of %lu",
                static_cast<unsigned long>(bad), static_cast<unsigned long>(total));
        }
        format_ok = chip_erase_and_mount();
        current_phase = Phase::done;
        return true;

    case Phase::done:
        return true;
    }
    return true;
}
#endif
