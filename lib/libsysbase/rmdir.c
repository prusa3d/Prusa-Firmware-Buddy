#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <reent.h>
#include <errno.h>
#include <sys/iosupport.h>

int	rmdir (const char *name) {
	struct _reent *r = _REENT;
	int	dev,ret=-1;

	dev	= FindDevice(name);
	if(dev!=-1) {
		if(devoptab_list[dev]->rmdir_r) {
			r->deviceData = devoptab_list[dev]->deviceData;
			ret = devoptab_list[dev]->rmdir_r(r,name);
		} else {
			r->_errno = ENOSYS;
		}
	} else {
		r->_errno =	ENODEV;
	}

	return ret;
}
