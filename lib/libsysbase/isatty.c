#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>

#ifdef REENTRANT_SYSCALLS_PROVIDED
//---------------------------------------------------------------------------------
int _isatty_r( struct _reent *ptr, int file) {
//---------------------------------------------------------------------------------
#else
//---------------------------------------------------------------------------------
int _isatty(int file) {
//---------------------------------------------------------------------------------
	struct _reent *ptr = _REENT;
#endif
	return 0;
}
