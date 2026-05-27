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
    static LittlefsFlashDevice<Mt29fFlash> internal_device(Mt29fFlash::instance());
    if (lfs_t *lfs = internal_device.init()) {
        internal_ns::mount(lfs);
    }
#endif
}
