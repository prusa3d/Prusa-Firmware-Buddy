/*
 * Stub version of fork.
 */

#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#ifdef REENTRANT_SYSCALLS_PROVIDED
//---------------------------------------------------------------------------------
int _fork_r (struct _reent * r) {
//---------------------------------------------------------------------------------
#else
//---------------------------------------------------------------------------------
int _fork(void) {
//---------------------------------------------------------------------------------
	struct _reent *r = _REENT;
#endif
	r->_errno = ENOSYS;
	return -1;
}
