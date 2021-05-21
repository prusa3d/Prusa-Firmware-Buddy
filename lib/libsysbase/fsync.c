#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/iosupport.h>

int fsync(int   fileDesc ) {
	int ret = -1;
	unsigned int dev = 0;
	unsigned int fd = -1;
	struct _reent *r = _REENT;

	__handle * handle;

	handle = __get_handle(fileDesc);

	if ( NULL == handle ) {
		errno = EINVAL;
		return ret;
	}

	dev = handle->device;

	if(devoptab_list[dev]->fsync_r) {
		r->deviceData = devoptab_list[dev]->deviceData;
		ret = devoptab_list[dev]->fsync_r(r, handle->fileStruct);
	} else
		r->_errno=ENOSYS;

	return ret;
}
