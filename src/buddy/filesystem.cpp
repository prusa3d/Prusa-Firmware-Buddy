#include <sys/iosupport.h>

#include <buddy/filesystem.h>
#include <buddy/filesystem_fatfs.h>
#include <buddy/filesystem_littlefs_internal.h>
#include <buddy/filesystem_root.h>
#include <buddy/filesystem_semihosting.h>
#include <buddy/libsysbase_syscalls.h>

#include "stm32f4xx.h"
#include <logging/log.hpp>

LOG_COMPONENT_DEF(FileSystem, logging::Severity::info);

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
