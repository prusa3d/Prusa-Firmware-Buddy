/*
 * Stub version of wait.
 */

#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>

//---------------------------------------------------------------------------------
#ifdef REENTRANT_SYSCALLS_PROVIDED
//---------------------------------------------------------------------------------
int _wait_r(struct _reent *r, int  *status) {
#else
//---------------------------------------------------------------------------------
int
int _wait_r(int  *status) {
	struct _reent *r = _REENT;
#endif
//---------------------------------------------------------------------------------
	r->_errno = ENOSYS;
	return -1;
}

