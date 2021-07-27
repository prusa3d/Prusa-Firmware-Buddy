#include <errno.h>
#include <time.h>
#include <sys/iosupport.h>

int nanosleep(const struct timespec *req, struct timespec *rem)
{
   if ( __syscalls.nanosleep ) {
      return __syscalls.nanosleep(req, rem);
   } else {
      *rem = *req;
      errno = ENOSYS;
      return -1;
   }
}
