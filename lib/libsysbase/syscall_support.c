#include <sys/iosupport.h>

//---------------------------------------------------------------------------------
__syscalls_t __syscalls = {
//---------------------------------------------------------------------------------
	NULL,	// sbrk
	NULL,	// exit
	NULL,	// gettod_r
	NULL,	// lock_init
	NULL,	// lock_acquire
	NULL,	// lock_try_acquire
	NULL,	// lock_release
	NULL,	// lock_close
	NULL,	// lock_init_recursive
	NULL,	// lock_acquire_recursive
	NULL,	// lock_try_acquire_recursive
	NULL,	// lock_release_recursive
	NULL,	// lock_close_recursive
	NULL,	// __getreent
	NULL,	// clock_gettime
	NULL,	// clock_settime
	NULL,	// clock_getres
	NULL,	// nanosleep
};

void __libc_lock_init(_LOCK_T *lock) {

	if ( __syscalls.lock_init ) {
		__syscalls.lock_init(lock);
	}

}

void __libc_lock_acquire(_LOCK_T *lock ) {

	if ( __syscalls.lock_acquire) {
		__syscalls.lock_acquire(lock);
	}
}

int __libc_lock_try_acquire(_LOCK_T *lock ) {

	if ( __syscalls.lock_acquire) {
		return __syscalls.lock_try_acquire(lock);
	} else {
		return 0;
	}
}

void __libc_lock_release(_LOCK_T *lock ) {

	if ( __syscalls.lock_release) {
		__syscalls.lock_release(lock);
	}
}

void __libc_lock_close(_LOCK_T *lock ) {

	if ( __syscalls.lock_close) {
		__syscalls.lock_close(lock);
	}
}



void __libc_lock_init_recursive(_LOCK_RECURSIVE_T *lock) {

	if ( __syscalls.lock_init_recursive ) {
		__syscalls.lock_init_recursive(lock);
	}

}

void __libc_lock_acquire_recursive(_LOCK_RECURSIVE_T *lock ) {

	if ( __syscalls.lock_acquire_recursive) {
		__syscalls.lock_acquire_recursive(lock);
	}
}

int __libc_lock_try_acquire_recursive(_LOCK_RECURSIVE_T *lock ) {

	if ( __syscalls.lock_acquire_recursive) {
		return __syscalls.lock_try_acquire_recursive(lock);
	} else {
		return 0;
	}
}

void __libc_lock_release_recursive(_LOCK_RECURSIVE_T *lock ) {

	if ( __syscalls.lock_release_recursive) {
		__syscalls.lock_release_recursive(lock);
	}
}

void __libc_lock_close_recursive(_LOCK_RECURSIVE_T *lock ) {

	if ( __syscalls.lock_close_recursive) {
		__syscalls.lock_close_recursive(lock);
	}
}

