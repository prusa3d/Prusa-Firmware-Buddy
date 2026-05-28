#include "filesystem_littlefs.hpp"
#include "filesystem_littlefs_device.hpp"
#include "littlefs_flash_device.hpp"

#include <common/w25x.hpp>
#include <option/has_internal_storage_flash.h>
#if HAS_INTERNAL_STORAGE_FLASH()
    #include <common/mt29f_flash.hpp>
#endif

DEFINE_LITTLEFS_DEVICE(system)

#if HAS_INTERNAL_STORAGE_FLASH()
DEFINE_LITTLEFS_DEVICE(internal)
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
    static LittlefsFlashDevice<Mt29fFlash> internal_device(Mt29fFlash::instance());
    // Retry mounting to rule out a transient IO error: the recovery below is
    // destructive and must not trigger on a bus hiccup
    lfs_t *lfs = nullptr;
    for (int attempt = 0; attempt < 3 && !lfs; attempt++) {
        lfs = internal_device.init();
    }
    if (lfs) {
        internal_ns::mount(lfs);
    } else {
        // First boot (factory pages have invalid ECC) or corrupted beyond
        // mounting, chip erase to get a valid ECC state
        Mt29fFlash::instance().chip_erase();
        if (int err = Mt29fFlash::instance().fetch_error()) {
            log_error(FileSystem, "MT29F chip erase failed: %d", err);
            return;
        }
        if (lfs_t *lfs2 = internal_device.init()) {
            internal_ns::mount(lfs2);
        }
    }
#endif
}

#if HAS_INTERNAL_STORAGE_FLASH()
void internal_storage_unmount() {
    internal_ns::unmount();
}
#endif
