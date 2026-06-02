#include "filesystem_littlefs.hpp"
#include "filesystem_littlefs_device.hpp"
#include "littlefs_flash_device.hpp"

#include <common/w25x.hpp>
#if HAS_INTERNAL_STORAGE_FLASH()
    #include <common/mt29f_flash.hpp>
#endif

DEFINE_LITTLEFS_DEVICE(system)

#if HAS_INTERNAL_STORAGE_FLASH()
DEFINE_LITTLEFS_DEVICE(internal)

static LittlefsFlashDevice<Mt29fFlash> &internal_storage_device_instance() {
    static LittlefsFlashDevice<Mt29fFlash> instance(Mt29fFlash::instance());
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

bool internal_storage_format() {
    if (!Mt29fFlash::instance().is_initialized()) {
        return false;
    }
    internal_storage_unmount();
    return chip_erase_and_mount();
}
#endif
