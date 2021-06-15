#include <sys/iosupport.h>

#include "filesystem.h"
#include "filesystem_fatfs.h"
#include "libsysbase_syscalls.h"

void filesystem_init() {
#if !defined(_RETARGETABLE_LOCKING)
    libsysbase_syscalls_init();
#endif
    int device = filesystem_fatfs_init();
    if (device != -1) {
        setDefaultDevice(device);
    }
}
