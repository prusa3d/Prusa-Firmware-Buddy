#include <sys/iosupport.h>

#include "filesystem.h"
#include "filesystem_fatfs.h"
#include "filesystem_root.h"
#include "libsysbase_syscalls.h"

void filesystem_init() {
#if !defined(_RETARGETABLE_LOCKING)
    libsysbase_syscalls_init();
#endif
    filesystem_fatfs_init();
    int device = filesystem_root_init();

    if (device != -1) {
        setDefaultDevice(device);
    }
}
