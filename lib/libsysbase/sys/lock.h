#ifndef __SYS_LOCK_H__
#define __SYS_LOCK_H__

#include <_ansi.h>
#include <stdint.h>

typedef int32_t _LOCK_T;

struct __lock_t {
	_LOCK_T lock;
	uint32_t thread_tag;
	uint32_t counter;
};

typedef struct __lock_t _LOCK_RECURSIVE_T;

extern void __libc_lock_init(_LOCK_T *lock);
extern void __libc_lock_init_recursive(_LOCK_RECURSIVE_T *lock);
extern void __libc_lock_close(_LOCK_T *lock);
extern void __libc_lock_close_recursive(_LOCK_RECURSIVE_T *lock);
extern void __libc_lock_acquire(_LOCK_T *lock);
extern void __libc_lock_acquire_recursive(_LOCK_RECURSIVE_T *lock);
extern void __libc_lock_release(_LOCK_T *lock);
extern void __libc_lock_release_recursive(_LOCK_RECURSIVE_T *lock);

/* Returns 0 for success and non-zero for failure */
extern int __libc_lock_try_acquire(_LOCK_T *lock);
extern int __libc_lock_try_acquire_recursive(_LOCK_RECURSIVE_T *lock);

#define __LOCK_INIT(CLASS,NAME) \
CLASS _LOCK_T NAME = 1;

#define __LOCK_INIT_RECURSIVE(CLASS,NAME) \
CLASS _LOCK_RECURSIVE_T NAME = {1,0,0};

#define __lock_init(NAME) \
	__libc_lock_init(&(NAME))

#define __lock_init_recursive(NAME) \
	__libc_lock_init_recursive(&(NAME))

#define __lock_close(NAME) \
	__libc_lock_close(&(NAME))

#define __lock_close_recursive(NAME) \
	__libc_lock_close_recursive(&(NAME))

#define __lock_acquire(NAME) \
	__libc_lock_acquire(&(NAME))

#define __lock_acquire_recursive(NAME) \
	__libc_lock_acquire_recursive(&(NAME))

#define __lock_try_acquire(NAME) \
	__libc_lock_try_acquire(&(NAME))

#define __lock_try_acquire_recursive(NAME) \
	__libc_lock_try_acquire_recursive(&(NAME))

#define __lock_release(NAME) \
	__libc_lock_release(&(NAME))

#define __lock_release_recursive(NAME) \
	__libc_lock_release_recursive(&(NAME))

#endif // __SYS_LOCK_H__
