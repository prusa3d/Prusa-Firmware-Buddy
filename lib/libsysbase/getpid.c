#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>

#ifdef REENTRANT_SYSCALLS_PROVIDED
//---------------------------------------------------------------------------------
int _getpid_r(struct _reent *ptr) {
//---------------------------------------------------------------------------------
#else
//---------------------------------------------------------------------------------
int _getpid(void) {
//---------------------------------------------------------------------------------
	struct _reent *ptr = _REENT;
#endif
	ptr->_errno = ENOSYS;
	return -1;
}

