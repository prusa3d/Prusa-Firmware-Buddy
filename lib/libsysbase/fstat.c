#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/iosupport.h>

#ifdef REENTRANT_SYSCALLS_PROVIDED
//---------------------------------------------------------------------------------
int _fstat_r(
			struct _reent * r,
			int fileDesc,
			struct stat *st) {
//---------------------------------------------------------------------------------
#else
//---------------------------------------------------------------------------------
int _fstat(
			int fileDesc,
			struct stat *st) {
//---------------------------------------------------------------------------------
	struct _reent *r = _REENT;
#endif
	int ret = -1;
	unsigned int dev = 0;

	__handle * handle = NULL;

	if(fileDesc!=-1) {
		handle = __get_handle(fileDesc);

		if ( NULL == handle ) return ret;

		dev = handle->device;

		if(devoptab_list[dev]->fstat_r) {
			r->deviceData = devoptab_list[dev]->deviceData;
			ret = devoptab_list[dev]->fstat_r(r,handle->fileStruct,st);
		} else {
			r->_errno = ENOSYS;
		}
	}
	return ret;
}
