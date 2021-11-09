#include "config.h"
#include <_ansi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <reent.h>
#include <unistd.h>


extern char *fake_heap_end;
extern char *fake_heap_start;

/* Register name faking - works in collusion with the linker.  */
register char * stack_ptr asm ("sp");

#ifdef REENTRANT_SYSCALLS_PROVIDED
//---------------------------------------------------------------------------------
void * _sbrk_r (struct _reent *ptr, ptrdiff_t incr) {
//---------------------------------------------------------------------------------
#else
//---------------------------------------------------------------------------------
caddr_t _sbrk (int incr) {
//---------------------------------------------------------------------------------
	struct _reent *ptr = _REENT;
#endif
	extern char   end asm ("__end__");	/* Defined by the linker.  */
	static char * heap_start;

	char *	prev_heap_start;
	char *	heap_end;

	if (heap_start == NULL) {
		if (fake_heap_start == NULL) {
			heap_start = &end;
		} else {
			heap_start = fake_heap_start;
		}
	}

	prev_heap_start = heap_start;

	if (fake_heap_end == NULL) {
		heap_end = stack_ptr;
	} else {
		heap_end = fake_heap_end;
	}

	if (heap_start + incr > heap_end) {
		ptr->_errno = ENOMEM;
		return (caddr_t) -1;
	}

	heap_start += incr;
	return (caddr_t) prev_heap_start;
}
