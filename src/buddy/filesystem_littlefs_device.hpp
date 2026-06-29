/// @file
/// Macro to generate littlefs devoptab trampolines and mount/unmount helpers.
///
/// Each invocation of DEFINE_LITTLEFS_DEVICE(name) generates a namespace
/// containing the devoptab trampoline functions, context, and mount/unmount
/// helpers for a littlefs device with the given name.
#pragma once

#include <buddy/filesystem_littlefs_posix.hpp>
#include <sys/iosupport.h>
#include <logging/log.hpp>

LOG_COMPONENT_REF(FileSystem);

// clang-format off
#define DEFINE_LITTLEFS_DEVICE(device_name)                                                                             \
namespace device_name##_ns {                                                                                            \
                                                                                                                        \
static filesystem_littlefs_ctx_t ctx;                                                                                   \
                                                                                                                        \
static int open_r(struct _reent *r, void *fs, const char *p, int f, int m) {                                            \
    return filesystem_littlefs_open_r(&ctx, r, fs, p, f, m);                                                            \
}                                                                                                                       \
static int close_r(struct _reent *r, void *fs) {                                                                        \
    return filesystem_littlefs_close_r(&ctx, r, fs);                                                                    \
}                                                                                                                       \
static int fsync_r(struct _reent *r, void *fs) {                                                                        \
    return filesystem_littlefs_fsync_r(&ctx, r, fs);                                                                    \
}                                                                                                                       \
static ssize_t write_r(struct _reent *r, void *fs, const char *ptr, size_t len) {                                       \
    return filesystem_littlefs_write_r(&ctx, r, fs, ptr, len);                                                          \
}                                                                                                                       \
static ssize_t read_r(struct _reent *r, void *fs, char *ptr, size_t len) {                                              \
    return filesystem_littlefs_read_r(&ctx, r, fs, ptr, len);                                                           \
}                                                                                                                       \
static off_t seek_r(struct _reent *r, void *fs, off_t pos, int dir) {                                                   \
    return filesystem_littlefs_seek_r(&ctx, r, fs, pos, dir);                                                           \
}                                                                                                                       \
static int stat_r(struct _reent *r, const char *p, struct stat *st) {                                                   \
    return filesystem_littlefs_stat_r(&ctx, r, p, st);                                                                  \
}                                                                                                                       \
static int fstat_r(struct _reent *r, void *fs, struct stat *st) {                                                       \
    return filesystem_littlefs_fstat_r(&ctx, r, fs, st);                                                                \
}                                                                                                                       \
static int link_r(struct _reent *r, const char *existing, const char *newLink) {                                        \
    return filesystem_littlefs_link_r(&ctx, r, existing, newLink);                                                      \
}                                                                                                                       \
static int unlink_r(struct _reent *r, const char *p) {                                                                  \
    return filesystem_littlefs_unlink_r(&ctx, r, p);                                                                    \
}                                                                                                                       \
static int chdir_r(struct _reent *r, const char *p) {                                                                   \
    return filesystem_littlefs_chdir_r(&ctx, r, p);                                                                     \
}                                                                                                                       \
static int rename_r(struct _reent *r, const char *old_name, const char *new_name) {                                     \
    return filesystem_littlefs_rename_r(&ctx, r, old_name, new_name);                                                   \
}                                                                                                                       \
static int chmod_r(struct _reent *r, const char *p, mode_t mode) {                                                      \
    return filesystem_littlefs_chmod_r(&ctx, r, p, mode);                                                               \
}                                                                                                                       \
static int fchmod_r(struct _reent *r, void *fs, mode_t mode) {                                                          \
    return filesystem_littlefs_fchmod_r(&ctx, r, fs, mode);                                                             \
}                                                                                                                       \
static int mkdir_r(struct _reent *r, const char *p, int mode) {                                                         \
    return filesystem_littlefs_mkdir_r(&ctx, r, p, mode);                                                               \
}                                                                                                                       \
static DIR_ITER *diropen_r(struct _reent *r, DIR_ITER *ds, const char *p) {                                             \
    return filesystem_littlefs_diropen_r(&ctx, r, ds, p);                                                               \
}                                                                                                                       \
static int dirreset_r(struct _reent *r, DIR_ITER *ds) {                                                                 \
    return filesystem_littlefs_dirreset_r(&ctx, r, ds);                                                                 \
}                                                                                                                       \
static int dirnext_r(struct _reent *r, DIR_ITER *ds, char *fn, struct stat *st) {                                       \
    return filesystem_littlefs_dirnext_r(&ctx, r, ds, fn, st);                                                          \
}                                                                                                                       \
static int dirclose_r(struct _reent *r, DIR_ITER *ds) {                                                                 \
    return filesystem_littlefs_dirclose_r(&ctx, r, ds);                                                                 \
}                                                                                                                       \
static int statvfs_r(struct _reent *r, const char *p, struct statvfs *buf) {                                            \
    return filesystem_littlefs_statvfs_r(&ctx, r, p, buf);                                                              \
}                                                                                                                       \
static int ftruncate_r(struct _reent *r, void *fs, off_t len) {                                                         \
    return filesystem_littlefs_ftruncate_r(&ctx, r, fs, len);                                                           \
}                                                                                                                       \
static int rmdir_r(struct _reent *r, const char *p) {                                                                   \
    return filesystem_littlefs_rmdir_r(&ctx, r, p);                                                                     \
}                                                                                                                       \
static int lstat_r(struct _reent *r, const char *p, struct stat *st) {                                                  \
    return filesystem_littlefs_lstat_r(&ctx, r, p, st);                                                                 \
}                                                                                                                       \
static int utimes_r(struct _reent *r, const char *p, const struct timeval times[2]) {                                   \
    return filesystem_littlefs_utimes_r(&ctx, r, p, times);                                                             \
}                                                                                                                       \
                                                                                                                        \
static const devoptab_t devoptab = {                                                                                    \
    .name = #device_name,                                                                                               \
    .structSize = sizeof(FIL_EX),                                                                                       \
    .open_r = open_r,                                                                                                   \
    .close_r = close_r,                                                                                                 \
    .write_r = write_r,                                                                                                 \
    .read_r = read_r,                                                                                                   \
    .seek_r = seek_r,                                                                                                   \
    .fstat_r = fstat_r,                                                                                                 \
    .stat_r = stat_r,                                                                                                   \
    .link_r = link_r,                                                                                                   \
    .unlink_r = unlink_r,                                                                                               \
    .chdir_r = chdir_r,                                                                                                 \
    .rename_r = rename_r,                                                                                               \
    .mkdir_r = mkdir_r,                                                                                                 \
    .dirStateSize = sizeof(lfs_dir_t),                                                                                  \
    .diropen_r = diropen_r,                                                                                             \
    .dirreset_r = dirreset_r,                                                                                           \
    .dirnext_r = dirnext_r,                                                                                             \
    .dirclose_r = dirclose_r,                                                                                           \
    .statvfs_r = statvfs_r,                                                                                             \
    .ftruncate_r = ftruncate_r,                                                                                         \
    .fsync_r = fsync_r,                                                                                                 \
    .chmod_r = chmod_r,                                                                                                 \
    .fchmod_r = fchmod_r,                                                                                               \
    .rmdir_r = rmdir_r,                                                                                                 \
    .lstat_r = lstat_r,                                                                                                 \
    .utimes_r = utimes_r,                                                                                               \
};                                                                                                                      \
                                                                                                                        \
int mount(lfs_t *lfs) {                                                                                                 \
    return filesystem_littlefs_mount(&ctx, &devoptab, lfs);                                                             \
}                                                                                                                       \
                                                                                                                        \
void unmount() {                                                                                                        \
    filesystem_littlefs_unmount(&ctx);                                                                                  \
}                                                                                                                       \
                                                                                                                        \
lfs_t *get_lfs() { return ctx.lfs; }                                                                                    \
                                                                                                                        \
}
// clang-format on
