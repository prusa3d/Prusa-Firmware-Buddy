//---------------------------------------------------------------------------------
#ifndef __iosupp_h__
#define __iosupp_h__
//---------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

#include <reent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>

enum	{
	STD_IN,
	STD_OUT,
	STD_ERR,
	STD_MAX = 35
};


typedef struct {
	unsigned int device;
	unsigned int refcount;
	void *fileStruct;
} __handle;

/* Directory iterator for mantaining state between dir* calls */
typedef struct {
    int device;
    void *dirStruct;
} DIR_ITER;

typedef struct {
	const char *name;
	size_t structSize;
	int (*open_r)(struct _reent *r, void *fileStruct, const char *path, int flags, int mode);
	int (*close_r)(struct _reent *r, void *fd);
	ssize_t (*write_r)(struct _reent *r, void *fd, const char *ptr, size_t len);
	ssize_t (*read_r)(struct _reent *r, void *fd, char *ptr, size_t len);
	off_t (*seek_r)(struct _reent *r, void *fd, off_t pos, int dir);
	int (*fstat_r)(struct _reent *r, void *fd, struct stat *st);
	int (*stat_r)(struct _reent *r, const char *file, struct stat *st);
	int (*link_r)(struct _reent *r, const char *existing, const char  *newLink);
	int (*unlink_r)(struct _reent *r, const char *name);
	int (*chdir_r)(struct _reent *r, const char *name);
	int (*rename_r) (struct _reent *r, const char *oldName, const char *newName);
	int (*mkdir_r) (struct _reent *r, const char *path, int mode);

	size_t dirStateSize;

	DIR_ITER* (*diropen_r)(struct _reent *r, DIR_ITER *dirState, const char *path);
	int (*dirreset_r)(struct _reent *r, DIR_ITER *dirState);
	int (*dirnext_r)(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat);
	int (*dirclose_r)(struct _reent *r, DIR_ITER *dirState);
	int (*statvfs_r)(struct _reent *r, const char *path, struct statvfs *buf);
	int (*ftruncate_r)(struct _reent *r, void *fd, off_t len);
	int (*fsync_r)(struct _reent *r, void *fd);

	void *deviceData;

	int (*chmod_r)(struct _reent *r, const char *path, mode_t mode);
	int (*fchmod_r)(struct _reent *r, void *fd, mode_t mode);
	int (*rmdir_r)(struct _reent *r, const char *name);
	int (*lstat_r)(struct _reent *r, const char *file, struct stat *st);
	int (*utimes_r)(struct _reent *r, const char *filename, const struct timeval times[2]);

} devoptab_t;

extern const devoptab_t *devoptab_list[];

typedef struct {
	void *(*sbrk_r) (struct _reent *ptr, ptrdiff_t incr);
	void (*exit) ( int rc );
	int  (*gettod_r) (struct _reent *ptr, struct timeval *tp, struct timezone *tz);
	void (*lock_init) (_LOCK_T *lock);
	void (*lock_acquire) (_LOCK_T *lock);
	int  (*lock_try_acquire) (_LOCK_T *lock);
	void (*lock_release) (_LOCK_T *lock);
	void (*lock_close) (_LOCK_T *lock);
	void (*lock_init_recursive) (_LOCK_RECURSIVE_T *lock);
	void (*lock_acquire_recursive) (_LOCK_RECURSIVE_T *lock);
	int  (*lock_try_acquire_recursive) (_LOCK_RECURSIVE_T *lock);
	void (*lock_release_recursive) (_LOCK_RECURSIVE_T *lock);
	void (*lock_close_recursive) (_LOCK_RECURSIVE_T *lock);
	struct _reent *(*getreent) ();
	int (*clock_gettime)(clockid_t clock_id, struct timespec *tp);
	int (*clock_settime)(clockid_t clock_id, const struct timespec *tp);
	int (*clock_getres)(clockid_t clock_id, struct timespec *res);
	int (*nanosleep)(const struct timespec *req, struct timespec *rem);
} __syscalls_t;

extern __syscalls_t __syscalls;

int AddDevice( const devoptab_t* device);
int FindDevice(const char* name);
int RemoveDevice(const char* name);
void setDefaultDevice( int device );
const devoptab_t* GetDeviceOpTab (const char *name);

void __release_handle(int fd);
int  __alloc_handle(int device);
__handle *__get_handle(int fd);

#ifdef __cplusplus
}
#endif

//---------------------------------------------------------------------------------
#endif // __iosupp_h__
//---------------------------------------------------------------------------------
