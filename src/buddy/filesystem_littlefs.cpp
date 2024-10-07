#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <sys/iosupport.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <buddy/filesystem.h>
#include <buddy/filesystem_littlefs.h>
#include <buddy/littlefs_internal.h>

#define RESULT result < 0 ? -1 : 0;

#define FLAG_SYNC 0b00000001

#define PREPARE_FIL_EX(f, fs) \
    FIL_EX *f = fs;           \
    if (f == NULL) {          \
        r->_errno = EBADF;    \
        return -1;            \
    }

#define device            (ctx->device)
#define devoptab_littlefs (*ctx->devoptab)

static int get_errno(int result) { // Only negative LFS results are errors
    if (result >= 0) {
        return 0;
    }

    switch (result) {
    case LFS_ERR_IO:
        return EIO;
    case LFS_ERR_CORRUPT:
        return EIO;
    case LFS_ERR_NOENT:
        return ENOENT;
    case LFS_ERR_EXIST:
        return EEXIST;
    case LFS_ERR_NOTDIR:
        return ENOTDIR;
    case LFS_ERR_ISDIR:
        return EISDIR;
    case LFS_ERR_NOTEMPTY:
        return ENOTEMPTY;
    case LFS_ERR_BADF:
        return EBADF;
    case LFS_ERR_FBIG:
        return EFBIG;
    case LFS_ERR_INVAL:
        return EINVAL;
    case LFS_ERR_NOSPC:
        return ENOSPC;
    case LFS_ERR_NOMEM:
        return ENOMEM;
    case LFS_ERR_NOATTR:
        return ENODATA;
    case LFS_ERR_NAMETOOLONG:
        return ENAMETOOLONG;
    default:
        return EINVAL;
    }
}

static enum lfs_open_flags get_littlefs_flags(int flags) {
    enum lfs_open_flags lfs_flags;

    switch (flags & O_ACCMODE) {
    case O_RDWR:
        lfs_flags = LFS_O_RDWR;
        break;
    case O_WRONLY:
        lfs_flags = LFS_O_WRONLY;
        break;
    case O_RDONLY:
    default:
        lfs_flags = LFS_O_RDONLY;
    }

    if (flags & O_APPEND) {
        lfs_flags = static_cast<lfs_open_flags>(lfs_flags | LFS_O_APPEND);
    }

    if (flags & O_CREAT) {
        lfs_flags = static_cast<lfs_open_flags>(lfs_flags | LFS_O_CREAT);
    }

    if (flags & O_EXCL) {
        lfs_flags = static_cast<lfs_open_flags>(lfs_flags | LFS_O_EXCL);
    }

    if (flags & O_TRUNC) {
        lfs_flags = static_cast<lfs_open_flags>(lfs_flags | LFS_O_TRUNC);
    }

    return lfs_flags;
}

static inline uint16_t crc32to16(uint32_t crc) {
    uint16_t crc1 = crc << 16;
    uint16_t crc2 = crc & 0xFFFF;
    return crc1 ^ crc2;
}

int filesystem_littlefs_open_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r,
    void *fileStruct,
    const char *path,
    int flags,
    [[maybe_unused]] int mode) {

    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    int result;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_littlefs.name);

    enum lfs_open_flags lfs_flags = get_littlefs_flags(flags);

    result = lfs_file_open(ctx->lfs, &(f->fil), path, lfs_flags);
    r->_errno = get_errno(result);

    if (result < 0) {
        return -1;
    }

    f->flags = 0;
    if (flags & O_SYNC) {
        f->flags |= FLAG_SYNC;
    }

    return 0;
}

int filesystem_littlefs_close_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct) {
    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    int result;
    result = lfs_file_close(ctx->lfs, &(f->fil));
    r->_errno = get_errno(result);

    return RESULT;
}

int filesystem_littlefs_fsync_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct) {
    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    int result;
    result = lfs_file_sync(ctx->lfs, &(f->fil));
    r->_errno = get_errno(result);

    return RESULT;
}

ssize_t filesystem_littlefs_write_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct, const char *ptr, size_t len) {
    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    int result;

    if (!ptr) {
        r->_errno = EINVAL;
        return -1;
    }

    if (len == 0) {
        r->_errno = 0;
        return 0;
    }

    result = lfs_file_write(ctx->lfs, &(f->fil), ptr, len);
    r->_errno = get_errno(result);

    if (result < 0) {
        return -1;
    }

    if (f->flags & FLAG_SYNC) {
        filesystem_littlefs_fsync_r(ctx, r, fileStruct);
    }

    return result;
}

ssize_t filesystem_littlefs_read_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct, char *ptr, size_t len) {
    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    int result;

    if (!ptr) {
        r->_errno = EINVAL;
        return -1;
    }

    if (len == 0) {
        r->_errno = 0;
        return 0;
    }

    result = lfs_file_read(ctx->lfs, &(f->fil), ptr, len);
    r->_errno = get_errno(result);

    if (result < 0) {
        return -1;
    }

    return result;
}

off_t filesystem_littlefs_seek_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct, off_t pos, int dir) {
    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    int result;

    result = lfs_file_seek(ctx->lfs, &(f->fil), pos, dir);
    r->_errno = get_errno(result);

    // this function is called from lseek not fseek.
    //  fseek returns 0 on successful move and lseek returns offset, where it moved.
    //  Thus we need to return offset not 0 on successful move
    return result;
}

int filesystem_littlefs_stat_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *path, struct stat *st) {
    int result;
    struct lfs_info info;
    memset(&info, 0, sizeof(struct lfs_info));
    memset(st, 0, sizeof(struct stat));

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_littlefs.name);

    result = lfs_stat(ctx->lfs, path, &info);
    r->_errno = get_errno(result);

    if (result < 0) {
        return -1;
    }

    st->st_mode = ALLPERMS; // Permissions handling not implemented, use all

    if (info.type & LFS_TYPE_DIR) {
        st->st_mode = S_IFDIR;
    } else if (info.type & LFS_TYPE_REG) {
        st->st_mode = S_IFREG;
    } else {
        // Unexpected type, return error
        r->_errno = EINVAL;
        return -1;
    }

    st->st_dev = device;
    st->st_size = info.size;
    st->st_blksize = ctx->lfs->cfg->block_size;
    st->st_blocks = 1;

    return 0;
}

int filesystem_littlefs_fstat_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct, struct stat *st) {
    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    memset(st, 0, sizeof(struct stat));

    st->st_mode = ALLPERMS; // Permissions handling not implemented, use all

    if (f->fil.type & LFS_TYPE_DIR) {
        st->st_mode = S_IFDIR;
    } else if (f->fil.type & LFS_TYPE_REG) {
        st->st_mode = S_IFREG;
    } else {
        // Unexpected type, return error
        r->_errno = EINVAL;
        return -1;
    }

    st->st_dev = device;
    st->st_size = f->fil.ctz.size;
    st->st_blksize = ctx->lfs->cfg->block_size;
    st->st_blocks = 1;

    return 0;
}

int filesystem_littlefs_link_r([[maybe_unused]] filesystem_littlefs_ctx_t *ctx, [[maybe_unused]] struct _reent *r,
    [[maybe_unused]] const char *existing,
    [[maybe_unused]] const char *newLink) {

    // Links are not supported on littlefs
    r->_errno = ENOTSUP;
    return -1;
}

int filesystem_littlefs_unlink_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *path) {
    int result;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_littlefs.name);

    result = lfs_remove(ctx->lfs, path);
    r->_errno = get_errno(result);

    return RESULT;
}

int filesystem_littlefs_chdir_r([[maybe_unused]] filesystem_littlefs_ctx_t *ctx, [[maybe_unused]] struct _reent *r, [[maybe_unused]] const char *path) {
    // chdir not implemented for littlefs
    r->_errno = ENOTSUP;
    return -1;
}

int filesystem_littlefs_rename_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *oldName, const char *newName) {
    int result;

    if (IS_EMPTY(oldName) || IS_EMPTY(newName)) {
        r->_errno = EINVAL;
        return -1;
    }

    oldName = process_path(oldName, devoptab_littlefs.name);
    newName = process_path(newName, devoptab_littlefs.name);

    result = lfs_rename(ctx->lfs, oldName, newName);
    r->_errno = get_errno(result);

    return RESULT;
}

int filesystem_littlefs_chmod_r([[maybe_unused]] filesystem_littlefs_ctx_t *ctx, [[maybe_unused]] struct _reent *r,
    [[maybe_unused]] const char *path,
    [[maybe_unused]] mode_t mode) {

    r->_errno = ENOTSUP;
    return -1;
}

int filesystem_littlefs_fchmod_r([[maybe_unused]] filesystem_littlefs_ctx_t *ctx, [[maybe_unused]] struct _reent *r,
    [[maybe_unused]] void *fileStruct,
    [[maybe_unused]] mode_t mode) {

    r->_errno = ENOTSUP;
    return -1;
}

int filesystem_littlefs_mkdir_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *path, [[maybe_unused]] int mode) {
    int result;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_littlefs.name);

    result = lfs_mkdir(ctx->lfs, path);
    r->_errno = get_errno(result);

    return RESULT;
}

DIR_ITER *filesystem_littlefs_diropen_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, DIR_ITER *dirState, const char *path) {
    int result;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return NULL;
    }

    path = process_path(path, devoptab_littlefs.name);

    result = lfs_dir_open(ctx->lfs, static_cast<lfs_dir_t *>(dirState->dirStruct), path);
    r->_errno = get_errno(result);

    if (result < 0) {
        return NULL;
    }

    return dirState;
}

int filesystem_littlefs_dirreset_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, DIR_ITER *dirState) {
    int result;

    result = lfs_dir_rewind(ctx->lfs, static_cast<lfs_dir_t *>(dirState->dirStruct));
    r->_errno = get_errno(result);

    return RESULT;
}

int filesystem_littlefs_dirnext_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat) {
    int result;
    struct lfs_info info;

    if (filename == NULL) {
        r->_errno = EINVAL;
        return -1;
    }

    result = lfs_dir_read(ctx->lfs, static_cast<lfs_dir_t *>(dirState->dirStruct), &info);
    r->_errno = get_errno(result);

    if (result == 0) {
        // End of the directory
        r->_errno = ENOENT; // has to be set at the end
        return -1;
    } else if (result < 0) {
        return -1;
    }

    if (info.type & LFS_TYPE_DIR) {
        if (filestat) {
            filestat->st_mode = S_IFDIR;
        }
    } else if (info.type & LFS_TYPE_REG) {
        if (filestat) {
            filestat->st_mode = S_IFREG;
        }
    } else {
        // Unexpected type, skip to the next dir
        return filesystem_littlefs_dirnext_r(ctx, r, dirState, filename, filestat);
    }

    strlcpy(filename, info.name, NAME_MAX);

    return 0;
}

int filesystem_littlefs_dirclose_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, DIR_ITER *dirState) {
    int result;

    result = lfs_dir_close(ctx->lfs, static_cast<lfs_dir_t *>(dirState->dirStruct));
    r->_errno = get_errno(result);

    return RESULT;
}

int filesystem_littlefs_statvfs_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *path, struct statvfs *buf) {
    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    memset(buf, 0, sizeof(struct statvfs));

    buf->f_frsize = ctx->lfs->cfg->block_size;
    buf->f_bfree = ctx->lfs->free.size;
    buf->f_bavail = buf->f_bfree;
    buf->f_files = 0; // TODO: Count all inodes
    buf->f_ffree = ctx->lfs->cfg->name_max - buf->f_files;
    buf->f_favail = buf->f_ffree;
    buf->f_fsid = (device & 0xFFFF) | crc32to16(ctx->lfs->seed); // 16b for filesystems, 16b for driver per filesystem
    buf->f_flag = ST_NOSUID;
    buf->f_namemax = ctx->lfs->cfg->name_max;

    return 0;
}

int filesystem_littlefs_ftruncate_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, void *fileStruct, off_t len) {
    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    int result;

    result = lfs_file_truncate(ctx->lfs, &(f->fil), len);
    r->_errno = get_errno(result);

    return RESULT;
}

int filesystem_littlefs_rmdir_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *path) {
    return filesystem_littlefs_unlink_r(ctx, r, path);
}

int filesystem_littlefs_lstat_r(filesystem_littlefs_ctx_t *ctx, struct _reent *r, const char *file, struct stat *st) {
    // littlefs doesn't support links, just return stat
    return filesystem_littlefs_stat_r(ctx, r, file, st);
}

int filesystem_littlefs_utimes_r([[maybe_unused]] filesystem_littlefs_ctx_t *ctx, [[maybe_unused]] struct _reent *r,
    [[maybe_unused]] const char *filename,
    [[maybe_unused]] const struct timeval times[2]) {

    // Timestamps not implemented for littlefs r->_errno = ENOTSUP;
    return -1;
}
