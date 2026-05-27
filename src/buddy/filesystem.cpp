#include <sys/iosupport.h>

#include <buddy/filesystem.h>
#include <buddy/filesystem_fatfs.h>
#include "filesystem_littlefs.hpp"
#include <buddy/filesystem_root.h>
#include <buddy/filesystem_semihosting.h>
#include <buddy/libsysbase_syscalls.h>
#include <common/sys.hpp>
#include <logging/log.hpp>

LOG_COMPONENT_DEF(FileSystem, logging::Severity::info);

void filesystem_init() {
#if !defined(_RETARGETABLE_LOCKING)
    // #error dead code found by automatic analyses (see BFW-5461)
    libsysbase_syscalls_init();
#endif

    filesystem_fatfs_init();
    filesystem_littlefs_init();

    // if debugger is attached, prepare semihosting fs
    if (sys_debugger_attached()) {
        filesystem_semihosting_init();
    }

    int device = filesystem_root_init();

    if (device != -1) {
        setDefaultDevice(device);
    }
}
void init_only_littlefs() {
#if !defined(_RETARGETABLE_LOCKING)
    // #error dead code found by automatic analyses (see BFW-5461)
    libsysbase_syscalls_init();
#endif

    filesystem_littlefs_init();
    int device = filesystem_root_init();
    if (device != -1) {
        setDefaultDevice(device);
    }
}
