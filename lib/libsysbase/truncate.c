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

	ret = -1;

	dev = FindDevice(file);

	if(dev!=-1 && devoptab_list[dev]->open_r && devoptab_list[dev]->close_r &&
	   devoptab_list[dev]->ftruncate_r)
	{

		fd = __alloc_handle(dev);

		if ( -1 != fd ) {
			handle = __get_handle(fd);

			ret = devoptab_list[dev]->open_r(r, handle->fileStruct, file, O_WRONLY, 0);

			if ( ret < 0 ) {
				__release_handle(fd);
				return ret;
			}

			ret = devoptab_list[dev]->ftruncate_r(r, handle->fileStruct, len);

			// Always close file and release handle
			devoptab_list[dev]->close_r(r, handle->fileStruct);
			__release_handle(fd);
		} else {
			r->_errno = ENOSR;
		}
	} else {
		r->_errno = ENOSYS;
	}

	return ret;
}
