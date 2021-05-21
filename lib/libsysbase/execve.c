/*
 * Stub version of execve.
 */

#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>

#ifdef REENTRANT_SYSCALLS_PROVIDED
//---------------------------------------------------------------------------------
int _execve_r(
        struct _reent *r,
        char  *name,
        char **argv,
        char **env) {
//---------------------------------------------------------------------------------
#else
//---------------------------------------------------------------------------------
int _execve(
        char  *name,
        char **argv,
        char **env) {
//---------------------------------------------------------------------------------
	struct _reent *r = _REENT;
#endif
	r->_errno = ENOSYS;
	return -1;
}

