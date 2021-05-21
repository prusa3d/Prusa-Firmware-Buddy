#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <reent.h>
#include <errno.h>
#include <sys/iosupport.h>

#ifdef REENTRANT_SYSCALLS_PROVIDED
//---------------------------------------------------------------------------------
int _unlink_r(struct _reent * r, const char *name ) {
//---------------------------------------------------------------------------------
#else
//---------------------------------------------------------------------------------
int _unlink_r(const char *name ) {
//---------------------------------------------------------------------------------
	struct _reent *r = _REENT;
#endif
	int dev,ret=-1;

	dev = FindDevice(name);
	if(dev<0) {
		r->_errno = ENODEV;
	} else {
		if (devoptab_list[dev]->unlink_r) {
			r->deviceData = devoptab_list[dev]->deviceData;
			ret = devoptab_list[dev]->unlink_r(r,name);
		} else {
			r->_errno=ENOSYS;
		}
	}

	return ret;
}

