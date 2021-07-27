#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/iosupport.h>
#include <errno.h>

#ifdef REENTRANT_SYSCALLS_PROVIDED
//---------------------------------------------------------------------------------
int _gettimeofday_r(
			struct _reent *ptr,
			struct timeval *ptimeval,
			void *ptimezone)
{
//---------------------------------------------------------------------------------
#else
//---------------------------------------------------------------------------------
int _gettimeofday(
        struct timeval  *ptimeval,
        void *ptimezone)
{
//---------------------------------------------------------------------------------
	struct _reent *ptr = _REENT;
#endif

	if ( __syscalls.gettod_r ) return __syscalls.gettod_r(ptr, ptimeval, ptimezone);

	ptr->_errno = ENOSYS;
	return -1;

}

