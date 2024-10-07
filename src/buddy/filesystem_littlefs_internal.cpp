#include <logging/log.hpp>
#include <buddy/filesystem_littlefs.h>
#include <sys/iosupport.h>
#include <buddy/littlefs_internal.h>

static filesystem_littlefs_ctx_t internal_ctx;

LOG_COMPONENT_REF(FileSystem);

static int open_r(struct _reent *r, void *fileStruct, const char *path, int flags, int mode) {
    return filesystem_littlefs_open_r(&internal_ctx, r, fileStruct, path, flags, mode);
}

static int close_r(struct _reent *r, void *fileStruct) {
    return filesystem_littlefs_close_r(&internal_ctx, r, fileStruct);
}

static int fsync_r(struct _reent *r, void *fileStruct) {
    return filesystem_littlefs_fsync_r(&internal_ctx, r, fileStruct);
}

static ssize_t write_r(struct _reent *r, void *fileStruct, const char *ptr, size_t len) {
    return filesystem_littlefs_write_r(&internal_ctx, r, fileStruct, ptr, len);
}

static ssize_t read_r(struct _reent *r, void *fileStruct, char *ptr, size_t len) {
    return filesystem_littlefs_read_r(&internal_ctx, r, fileStruct, ptr, len);
}

static off_t seek_r(struct _reent *r, void *fileStruct, off_t pos, int dir) {
    return filesystem_littlefs_seek_r(&internal_ctx, r, fileStruct, pos, dir);
}

static int stat_r(struct _reent *r, const char *path, struct stat *st) {
    return filesystem_littlefs_stat_r(&internal_ctx, r, path, st);
}

static int fstat_r(struct _reent *r, void *fileStruct, struct stat *st) {
    return filesystem_littlefs_fstat_r(&internal_ctx, r, fileStruct, st);
}

static int link_r(struct _reent *r, const char *existing, const char *newLink) {
    return filesystem_littlefs_link_r(&internal_ctx, r, existing, newLink);
}

static int unlink_r(struct _reent *r, const char *path) {
    return filesystem_littlefs_unlink_r(&internal_ctx, r, path);
}

static int chdir_r(struct _reent *r, __attribute__((unused)) const char *path) {
    return filesystem_littlefs_chdir_r(&internal_ctx, r, path);
}

static int rename_r(struct _reent *r, const char *oldName, const char *newName) {
    return filesystem_littlefs_rename_r(&internal_ctx, r, oldName, newName);
}

static int chmod_r(struct _reent *r, const char *path, mode_t mode) {
    return filesystem_littlefs_chmod_r(&internal_ctx, r, path, mode);
}

static int fchmod_r(struct _reent *r, void *fileStruct, mode_t mode) {
    return filesystem_littlefs_fchmod_r(&internal_ctx, r, fileStruct, mode);
}

static int mkdir_r(struct _reent *r, const char *path, int mode) {
    return filesystem_littlefs_mkdir_r(&internal_ctx, r, path, mode);
}

static DIR_ITER *diropen_r(struct _reent *r, DIR_ITER *dirState, const char *path) {
    return filesystem_littlefs_diropen_r(&internal_ctx, r, dirState, path);
}

static int dirreset_r(struct _reent *r, DIR_ITER *dirState) {
    return filesystem_littlefs_dirreset_r(&internal_ctx, r, dirState);
}

static int dirnext_r(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat) {
    return filesystem_littlefs_dirnext_r(&internal_ctx, r, dirState, filename, filestat);
}

static int dirclose_r(struct _reent *r, DIR_ITER *dirState) {
    return filesystem_littlefs_dirclose_r(&internal_ctx, r, dirState);
}

static int statvfs_r(struct _reent *r, const char *path, struct statvfs *buf) {
    return filesystem_littlefs_statvfs_r(&internal_ctx, r, path, buf);
}

static int ftruncate_r(struct _reent *r, void *fileStruct, off_t len) {
    return filesystem_littlefs_ftruncate_r(&internal_ctx, r, fileStruct, len);
}

static int rmdir_r(struct _reent *r, const char *path) {
    return filesystem_littlefs_rmdir_r(&internal_ctx, r, path);
}

static int lstat_r(struct _reent *r, const char *file, struct stat *st) {
    return filesystem_littlefs_lstat_r(&internal_ctx, r, file, st);
}

static int utimes_r(struct _reent *r, const char *filename, const struct timeval times[2]) {
    return filesystem_littlefs_utimes_r(&internal_ctx, r, filename, times);
}

static const devoptab_t internal_devoptab = {
    .name = "internal",
    .structSize = sizeof(FIL_EX),
    .open_r = open_r,
    .close_r = close_r,
    .write_r = write_r,
    .read_r = read_r,
    .seek_r = seek_r,
    .fstat_r = fstat_r,
    .stat_r = stat_r,
    .link_r = link_r,
    .unlink_r = unlink_r,
    .chdir_r = chdir_r,
    .rename_r = rename_r,
    .mkdir_r = mkdir_r,
    .dirStateSize = sizeof(lfs_dir_t),
    .diropen_r = diropen_r,
    .dirreset_r = dirreset_r,
    .dirnext_r = dirnext_r,
    .dirclose_r = dirclose_r,
    .statvfs_r = statvfs_r,
    .ftruncate_r = ftruncate_r,
    .fsync_r = fsync_r,
    .chmod_r = chmod_r,
    .fchmod_r = fchmod_r,
    .rmdir_r = rmdir_r,
    .lstat_r = lstat_r,
    .utimes_r = utimes_r,
};

extern "C" int filesystem_littlefs_internal_init() {
    if (internal_ctx.device > 0) {
        // Already initialized
        return internal_ctx.device;
    }

    lfs_t *lfs = littlefs_internal_init();
    if (lfs == NULL) {
        return -1;
    }

    internal_ctx.devoptab = &internal_devoptab;
    internal_ctx.lfs = lfs;

    int device = AddDevice(&internal_devoptab);
    internal_ctx.device = device;

    if (internal_ctx.device == -1) {
        log_error(FileSystem, "Failed to initialize LittleFS");
    } else {
        log_info(FileSystem, "LittleFS successfully initialized (device %i)", internal_ctx.device);
    }

    return internal_ctx.device;
}
