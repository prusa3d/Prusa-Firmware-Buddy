#include <malloc.h>
#include <sys/lock.h>
#include "cmsis_os.h"
#include "bsod.h"

#undef _RETARGETABLE_LOCKING

#if !defined(_RETARGETABLE_LOCKING)
    #include <sys/iosupport.h>
#endif

osMutexDef(libsysbase_mutex);

#if defined(_RETARGETABLE_LOCKING)
// This part is not yet tested

struct __lock {
    osMutexId mutexId;
};

struct __lock __lock___hndl_lock;

__attribute__((constructor)) static void init_retarget_locks(void) {
    __lock___hndl_lock.mutexId = osMutexCreate(osMutex(libsysbase_mutex));
}

void __retarget_lock_init(_LOCK_T *lock_ptr) {
    *lock_ptr = (_LOCK_T)malloc(sizeof(struct __lock));
    (*lock_ptr)->mutexId = osMutexCreate(osMutex(libsysbase_mutex));
}

void __retarget_lock_init_recursive(_LOCK_T *lock_ptr) {
    bsod("Not implemented");
}

void __retarget_lock_close(_LOCK_T lock) {
    osMutexDelete(lock->mutexId);
    free(lock);
}

void __retarget_lock_close_recursive(_LOCK_T lock) {
    bsod("Not implemented");
}

void __retarget_lock_acquire(_LOCK_T lock) {
    if (lock == NULL || lock->mutexId == 0) {
        return;
    }
    osMutexWait(lock->mutexId, osWaitForever);
}

void __retarget_lock_acquire_recursive(_LOCK_T lock) {
    bsod("Not implemented");
}

int __retarget_lock_try_acquire(_LOCK_T lock) {
    bsod("Not implemented");
    return -1;
}

int __retarget_lock_try_acquire_recursive(_LOCK_T lock) {
    bsod("Not implemented");
    return -1;
}

void __retarget_lock_release(_LOCK_T lock) {
    if (lock == NULL || lock->mutexId == 0) {
        return;
    }
    osMutexRelease(lock->mutexId);
}

void __retarget_lock_release_recursive(_LOCK_T lock) {
    bsod("Not implemented");
}

#else

osMutexId libsysbase_mutex_id;

static osMutexId getMutexId(_LOCK_T *lock) {
    // __LOCK_INIT sets _LOCK_T value to 1, use shared lock in that case.
    // Othervise we are expecting pointer of newly initialized lock to be set.
    return *lock == 1 ? libsysbase_mutex_id : (osMutexId)*lock;
}

void lock_init(_LOCK_T *lock) {
    *lock = (_LOCK_T)osMutexCreate(osMutex(libsysbase_mutex));
}

void lock_acquire(_LOCK_T *lock) {
    osMutexWait(getMutexId(lock), osWaitForever);
}

int lock_try_acquire(__attribute__((unused)) _LOCK_T *lock) {
    bsod("Not implemented");
    return -1;
}
void lock_release(_LOCK_T *lock) {
    osMutexRelease(getMutexId(lock));
}

void lock_close(_LOCK_T *lock) {
    if (*lock == 1) {
        bsod("Unable to close default lock");
        return;
    }
    osMutexDelete((osMutexId)*lock);
}

void lock_init_recursive(__attribute__((unused)) _LOCK_RECURSIVE_T *lock) {
    bsod("Not implemented");
}

void lock_acquire_recursive(__attribute__((unused)) _LOCK_RECURSIVE_T *lock) {
    bsod("Not implemented");
}

int lock_try_acquire_recursive(__attribute__((unused)) _LOCK_RECURSIVE_T *lock) {
    bsod("Not implemented");
    return -1;
}

void lock_release_recursive(__attribute__((unused)) _LOCK_RECURSIVE_T *lock) {
    bsod("Not implemented");
}

void lock_close_recursive(__attribute__((unused)) _LOCK_RECURSIVE_T *lock) {
    bsod("Not implemented");
}

// Linker optimize out objects with functions called only by newlib.
// Added dummy functions and call them to keep those objects linked.
extern "C" void keep_fstat();
extern "C" void keep_link();
extern "C" void keep_lseek();
extern "C" void keep_open();
extern "C" void keep_read();
extern "C" void keep_rename();
extern "C" void keep_stat();
extern "C" void keep_unlink();
extern "C" void keep_write();

void libsysbase_syscalls_init() {
    #if !defined(_RETARGETABLE_LOCKING)
    libsysbase_mutex_id = osMutexCreate(osMutex(libsysbase_mutex));
    #endif

    __syscalls.lock_init = lock_init;
    __syscalls.lock_acquire = lock_acquire;
    __syscalls.lock_try_acquire = lock_try_acquire;
    __syscalls.lock_release = lock_release;
    __syscalls.lock_close = lock_close;
    __syscalls.lock_init_recursive = lock_init_recursive;
    __syscalls.lock_acquire_recursive = lock_acquire_recursive;
    __syscalls.lock_try_acquire_recursive = lock_try_acquire_recursive;
    __syscalls.lock_release_recursive = lock_release_recursive;
    __syscalls.lock_close_recursive = lock_close_recursive;

    keep_fstat();
    keep_link();
    keep_lseek();
    keep_open();
    keep_read();
    keep_rename();
    keep_stat();
    keep_unlink();
    keep_write();
}

#endif
