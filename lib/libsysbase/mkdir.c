#include <reent.h>
#include <sys/iosupport.h>
#include <errno.h>

int mkdir (const char *path, mode_t mode) {
	struct _reent *r = _REENT;
	int ret;
	int dev = FindDevice(path);
	ret = -1;

	if (devoptab_list[dev]->mkdir_r) {
		r->deviceData = devoptab_list[dev]->deviceData;
		ret = devoptab_list[dev]->mkdir_r(r, path, mode);
	} else {
		r->_errno = ENOSYS;
	}

	return ret;
}
