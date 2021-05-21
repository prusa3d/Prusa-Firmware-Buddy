#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <utime.h>
#include <sys/iosupport.h>

int utimes(const char *filename, const struct timeval times[2])
{
        struct _reent *r = _REENT;
        int dev,ret;

        dev = FindDevice(filename);

        if(dev!=-1) {
                if (devoptab_list[dev]->utimes_r) {
                        r->deviceData = devoptab_list[dev]->deviceData;
                        ret = devoptab_list[dev]->utimes_r(r,filename,times);
                } else {
                        r->_errno=ENOSYS;
                }
        } else {
                ret = -1;
                r->_errno = ENODEV;
        }
        return ret;



}


int utime(const char *filename, const struct utimbuf *times)
{
        struct timeval t[2];
        if (times) {
                t[0].tv_sec  = times->actime;
                t[0].tv_usec = 0;
                t[1].tv_sec  = times->modtime;
                t[1].tv_usec = 0;
        }

        return utimes(filename, t);
}
