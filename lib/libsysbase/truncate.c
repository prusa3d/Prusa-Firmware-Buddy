#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/iosupport.h>

int truncate(const char *file, off_t len)
{

	__handle *handle;
	int dev, fd, ret;

	struct _reent * r = _REENT;

	dev = FindDevice(file);

	if(dev!=-1 && devoptab_list[dev]->open_r && devoptab_list[dev]->close_r &&
	   devoptab_list[dev]->ftruncate_r)
	{

		fd = __alloc_handle(dev);

		if ( -1 != fd ) {
			handle = __get_handle(fd);

			r->deviceData = devoptab_list[dev]->deviceData;

			ret = devoptab_list[dev]->open_r(r, handle->fileStruct, file, O_WRONLY, 0);

			if ( ret < 0 ) {
				__release_handle(fd);
				return ret;
			}

			ret = devoptab_list[dev]->ftruncate_r(r, handle->fileStruct, len);

			if (ret >= 0) {
				ret = devoptab_list[dev]->close_r(r, handle->fileStruct);
			} else {
				// Close it anyway, we don't want to leak memory
				devoptab_list[dev]->close_r(r, handle->fileStruct);
			}
		} else {
			r->_errno = ENOSR;
		}
	} else {
		r->_errno = ENOSYS;
	}

	return ret;
}
