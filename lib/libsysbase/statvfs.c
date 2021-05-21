#include <reent.h>
#include <sys/iosupport.h>
#include <errno.h>


int statvfs(const char *path, struct statvfs *buf) {
	struct _reent *r = _REENT;

	int ret;
	int device = FindDevice(path);

	ret = -1;

	if ( device != -1 && devoptab_list[device]->statvfs_r) {

		r->deviceData = devoptab_list[device]->deviceData;
		ret = devoptab_list[device]->statvfs_r(r, path, buf );

	} else {
		r->_errno = ENOSYS;
	}

	return ret;
}
