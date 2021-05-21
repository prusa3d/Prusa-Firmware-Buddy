#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/iosupport.h>
#include <errno.h>

#include <sys/iosupport.h>

int fchmod(int fd, mode_t mode) {
	int	ret = -1, dev;
	struct _reent *r = _REENT;

	if(fd!=-1) {

		__handle *handle = __get_handle(fd);

		if ( handle != NULL) {

			dev = handle->device;

			if(devoptab_list[dev]->fchmod_r) {
				r->deviceData = devoptab_list[dev]->deviceData;
				ret = devoptab_list[dev]->fchmod_r(r,handle->fileStruct,mode);
			} else
				r->_errno=ENOSYS;
		}
	}
	return ret;
}
