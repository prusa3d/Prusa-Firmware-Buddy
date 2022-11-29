#include <sys/iosupport.h>

#include "filesystem.h"
#include "filesystem_fatfs.h"
#include "filesystem_littlefs_internal.h"
#include "filesystem_root.h"
#include "filesystem_semihosting.h"
#include "libsysbase_syscalls.h"

#include "stm32f4xx.h"
#include "log.h"

LOG_COMPONENT_DEF(FileSystem, LOG_SEVERITY_INFO);

void filesystem_init() {
#if !defined(_RETARGETABLE_LOCKING)
    libsysbase_syscalls_init();
#endif

    filesystem_fatfs_init();
    filesystem_littlefs_internal_init();

    // if debugger is attached, prepare semihosting fs
    if (DBGMCU->CR != 0) {
        filesystem_semihosting_init();
    }

    int device = filesystem_root_init();

    if (device != -1) {
        setDefaultDevice(device);
    }
}
void init_only_littlefs() {
#if !defined(_RETARGETABLE_LOCKING)
    libsysbase_syscalls_init();
#endif

    filesystem_littlefs_internal_init();
    int device = filesystem_root_init();
    if (device != -1) {
        setDefaultDevice(device);
    }
}
