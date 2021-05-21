#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/iosupport.h>
#include <errno.h>


//---------------------------------------------------------------------------------
int lstat (const char *__restrict __path, struct stat *__restrict __buf ) {
//---------------------------------------------------------------------------------
	struct _reent *r = _REENT;
	int dev,ret;

	dev = FindDevice(__path);

	if(dev!=-1) {
		if (devoptab_list[dev]->lstat_r) {
			r->deviceData = devoptab_list[dev]->deviceData;
			ret = devoptab_list[dev]->lstat_r(r,__path,__buf);
		} else {
			r->_errno=ENOSYS;
		}
	} else {
		ret = -1;
		r->_errno = ENODEV;
	}
	return ret;
}

