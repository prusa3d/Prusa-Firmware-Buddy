#pragma once

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

#include "lfs.h"
#include "sys/iosupport.h"

typedef struct {
    int device;
    const devoptab_t *devoptab;
    lfs_t *lfs;
} filesystem_littlefs_ctx_t;

typedef struct {
    lfs_file_t fil;
    uint8_t flags;
} FIL_EX;

int filesystem_littlefs_open_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct, const char *path, int flags, int mode);
int filesystem_littlefs_close_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct);
int filesystem_littlefs_fsync_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct);
ssize_t filesystem_littlefs_write_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct, const char *ptr, size_t len);
ssize_t filesystem_littlefs_read_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct, char *ptr, size_t len);
off_t filesystem_littlefs_seek_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct, off_t pos, int dir);
int filesystem_littlefs_stat_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *path, struct stat *st);
int filesystem_littlefs_fstat_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct, struct stat *st);
int filesystem_littlefs_link_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *existing, const char *newLink);
int filesystem_littlefs_unlink_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *path);
int filesystem_littlefs_chdir_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *path);
int filesystem_littlefs_rename_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *oldName, const char *newName);
int filesystem_littlefs_chmod_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *path, mode_t mode);
int filesystem_littlefs_fchmod_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct, mode_t mode);
int filesystem_littlefs_mkdir_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *path, int mode);
DIR_ITER *filesystem_littlefs_diropen_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, DIR_ITER *dirState, const char *path);
int filesystem_littlefs_dirreset_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, DIR_ITER *dirState);
int filesystem_littlefs_dirnext_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat);
int filesystem_littlefs_dirclose_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, DIR_ITER *dirState);
int filesystem_littlefs_statvfs_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *path, struct statvfs *buf);
int filesystem_littlefs_ftruncate_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct, off_t len);
int filesystem_littlefs_rmdir_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *path);
int filesystem_littlefs_lstat_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *file, struct stat *st);
int filesystem_littlefs_utimes_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *filename, const struct timeval times[2]);

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)
