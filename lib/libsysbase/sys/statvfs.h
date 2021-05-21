#ifndef _SYS_STATVFS_H
#define _SYS_STATVFS_H

#define ST_RDONLY 0x0001
#define ST_NOSUID 0x0002

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

struct statvfs {
	unsigned long f_bsize;
	unsigned long f_frsize;
	fsblkcnt_t f_blocks;
	fsblkcnt_t f_bfree;
	fsblkcnt_t f_bavail;
	fsfilcnt_t f_files;
	fsfilcnt_t f_ffree;
	fsfilcnt_t f_favail;
	unsigned long f_fsid;
	unsigned long f_flag;
	unsigned long f_namemax;
}; 

int statvfs(const char *path, struct statvfs *buf);

#ifdef __cplusplus
}
#endif

#endif // _SYS_STATVFS_H
