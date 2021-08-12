#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <sys/iosupport.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include "filesystem.h"
#include "filesystem_littlefs.h"
#include "littlefs.h"

#define RESULT result < 0 ? -1 : 0;

#define FLAG_SYNC 0b00000001

#define PREPARE_FIL_EX(f, fs) \
    FIL_EX *f = fs;           \
    if (f == NULL) {          \
        r->_errno = EBADF;    \
        return -1;            \
    }

typedef struct {
    lfs_file_t fil;
    uint8_t flags;
} FIL_EX;

static lfs_t *lfs;

static int device = -1;

static const devoptab_t devoptab_littlefs;

static int get_errno(int result) {
    // Only negative LFS results are errors
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
        lfs_flags |= LFS_O_APPEND;
    }

    if (flags & O_CREAT) {
        lfs_flags |= LFS_O_CREAT;
    }

    if (flags & O_EXCL) {
        lfs_flags |= LFS_O_EXCL;
    }

    if (flags & O_TRUNC) {
        lfs_flags |= LFS_O_TRUNC;
    }

    return lfs_flags;
}

static inline uint16_t crc32to16(uint32_t crc) {
    uint16_t crc1 = crc << 16;
    uint16_t crc2 = crc & 0xFFFF;
    return crc1 ^ crc2;
}

static int open_r(struct _reent *r,
    void *fileStruct,
    const char *path,
    int flags,
    __attribute__((unused)) int mode) {

    PREPARE_FIL_EX(f, fileStruct);
    int result;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_littlefs.name);

    enum lfs_open_flags lfs_flags = get_littlefs_flags(flags);

    result = lfs_file_open(lfs, &(f->fil), path, lfs_flags);
    r->_errno = get_errno(result);

    if (result < 0) {
        return -1;
    }

    if (flags & O_SYNC) {
        f->flags |= FLAG_SYNC;
    }

    return 0;
}

static int close_r(struct _reent *r, void *fileStruct) {
    PREPARE_FIL_EX(f, fileStruct);
    int result;
    result = lfs_file_close(lfs, &(f->fil));
    r->_errno = get_errno(result);

    return RESULT;
}

static int fsync_r(struct _reent *r, void *fileStruct) {
    PREPARE_FIL_EX(f, fileStruct);
    int result;
    result = lfs_file_sync(lfs, &(f->fil));
    r->_errno = get_errno(result);

    return RESULT;
}

static ssize_t write_r(struct _reent *r, void *fileStruct, const char *ptr, size_t len) {
    PREPARE_FIL_EX(f, fileStruct);
    int result;

    if (!ptr) {
        r->_errno = EINVAL;
        return -1;
    }

    if (len == 0) {
        r->_errno = 0;
        return 0;
    }

    result = lfs_file_write(lfs, &(f->fil), ptr, len);
    r->_errno = get_errno(result);

    if (result < 0) {
        return -1;
    }

    if (f->flags & FLAG_SYNC) {
        fsync_r(r, fileStruct);
    }

    return result;
}

static ssize_t read_r(struct _reent *r, void *fileStruct, char *ptr, size_t len) {
    PREPARE_FIL_EX(f, fileStruct);
    int result;

    if (!ptr) {
        r->_errno = EINVAL;
        return -1;
    }

    if (len == 0) {
        r->_errno = 0;
        return 0;
    }

    result = lfs_file_read(lfs, &(f->fil), ptr, len);
    r->_errno = get_errno(result);

    if (result < 0) {
        return -1;
    }

    return result;
}

static off_t seek_r(struct _reent *r, void *fileStruct, off_t pos, int dir) {
    PREPARE_FIL_EX(f, fileStruct);
    int result;

    result = lfs_file_seek(lfs, &(f->fil), pos, dir);
    r->_errno = get_errno(result);

    //this function is called from lseek not fseek.
    // fseek returns 0 on successful move and lseek returns offset, where it moved.
    // Thus we need to return offset not 0 on successful move
    return result;
}

static int stat_r(struct _reent *r, const char *path, struct stat *st) {
    int result;
    struct lfs_info info;
    memset(&info, 0, sizeof(struct lfs_info));
    memset(st, 0, sizeof(struct stat));

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_littlefs.name);

    result = lfs_stat(lfs, path, &info);
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
    st->st_blksize = lfs->cfg->block_size;
    st->st_blocks = 1;

    return 0;
}

static int fstat_r(struct _reent *r, void *fileStruct, struct stat *st) {
    PREPARE_FIL_EX(f, fileStruct);
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
    st->st_blksize = lfs->cfg->block_size;
    st->st_blocks = 1;

    return 0;
}

static int link_r(struct _reent *r,
    __attribute__((unused)) const char *existing,
    __attribute__((unused)) const char *newLink) {

    // Links are not supported on littlefs
    r->_errno = ENOTSUP;
    return -1;
}

static int unlink_r(struct _reent *r, const char *path) {
    int result;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_littlefs.name);

    result = lfs_remove(lfs, path);
    r->_errno = get_errno(result);

    return RESULT;
}

static int chdir_r(struct _reent *r, __attribute__((unused)) const char *path) {
    // chdir not implemented for littlefs
    r->_errno = ENOTSUP;
    return -1;
}

static int rename_r(struct _reent *r, const char *oldName, const char *newName) {
    int result;

    if (IS_EMPTY(oldName) || IS_EMPTY(newName)) {
        r->_errno = EINVAL;
        return -1;
    }

    oldName = process_path(oldName, devoptab_littlefs.name);
    newName = process_path(newName, devoptab_littlefs.name);

    result = lfs_rename(lfs, oldName, newName);
    r->_errno = get_errno(result);

    return RESULT;
}

static int chmod_r(struct _reent *r,
    __attribute__((unused)) const char *path,
    __attribute__((unused)) mode_t mode) {

    r->_errno = ENOTSUP;
    return -1;
}

static int fchmod_r(struct _reent *r,
    __attribute__((unused)) void *fileStruct,
    __attribute__((unused)) mode_t mode) {

    r->_errno = ENOTSUP;
    return -1;
}

static int mkdir_r(struct _reent *r, const char *path, __attribute__((unused)) int mode) {
    int result;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_littlefs.name);

    result = lfs_mkdir(lfs, path);
    r->_errno = get_errno(result);

    return RESULT;
}

static DIR_ITER *diropen_r(struct _reent *r, DIR_ITER *dirState, const char *path) {
    int result;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return NULL;
    }

    path = process_path(path, devoptab_littlefs.name);

    result = lfs_dir_open(lfs, dirState->dirStruct, path);
    r->_errno = get_errno(result);

    if (result < 0) {
        return NULL;
    }

    return dirState;
}

static int dirreset_r(struct _reent *r, DIR_ITER *dirState) {
    int result;

    result = lfs_dir_rewind(lfs, dirState->dirStruct);
    r->_errno = get_errno(result);

    return RESULT;
}

static int dirnext_r(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat) {
    int result;
    struct lfs_info info;

    if (filename == NULL || filestat == NULL) {
        r->_errno = EINVAL;
        return -1;
    }

    result = lfs_dir_read(lfs, dirState->dirStruct, &info);
    r->_errno = get_errno(result);

    if (result == 0) {
        // End of the directory
        r->_errno = ENOENT; // has to be set at the end
        return -1;
    } else if (result < 0) {
        return -1;
    }

    if (info.type & LFS_TYPE_DIR) {
        filestat->st_mode = S_IFDIR;
    } else if (info.type & LFS_TYPE_REG) {
        filestat->st_mode = S_IFREG;
    } else {
        // Unexpected type, skip to the next dir
        return dirnext_r(r, dirState, filename, filestat);
    }

    strncpy(filename, info.name, NAME_MAX);

    return 0;
}

static int dirclose_r(struct _reent *r, DIR_ITER *dirState) {
    int result;

    result = lfs_dir_close(lfs, dirState->dirStruct);
    r->_errno = get_errno(result);

    return RESULT;
}

static int statvfs_r(struct _reent *r, const char *path, struct statvfs *buf) {
    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    memset(buf, 0, sizeof(struct statvfs));

    buf->f_frsize = lfs->cfg->block_size;
    buf->f_bfree = lfs->free.size;
    buf->f_bavail = buf->f_bfree;
    buf->f_files = 0; // TODO: Count all inodes
    buf->f_ffree = lfs->cfg->name_max - buf->f_files;
    buf->f_favail = buf->f_ffree;
    buf->f_fsid = (device & 0xFFFF) | crc32to16(lfs->seed); // 16b for filesystems, 16b for driver per filesystem
    buf->f_flag = ST_NOSUID;
    buf->f_namemax = lfs->cfg->name_max;

    return 0;
}

static int ftruncate_r(struct _reent *r, void *fileStruct, off_t len) {
    PREPARE_FIL_EX(f, fileStruct);
    int result;

    result = lfs_file_truncate(lfs, &(f->fil), len);
    r->_errno = get_errno(result);

    return RESULT;
}

static int rmdir_r(struct _reent *r, const char *path) {
    return unlink_r(r, path);
}

static int lstat_r(struct _reent *r, const char *file, struct stat *st) {
    // littlefs doesn't support links, just return stat
    return stat_r(r, file, st);
}

static int utimes_r(struct _reent *r,
    __attribute__((unused)) const char *filename,
    __attribute__((unused)) const struct timeval times[2]) {

    // Timestamps not implemented for littlefs
    r->_errno = ENOTSUP;
    return -1;
}

static const devoptab_t devoptab_littlefs = {
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

int filesystem_littlefs_init() {
    if (device != -1) {
        // Already initialized
        return device;
    }

    lfs = littlefs_init();
    if (lfs == NULL) {
        return -1;
    }

    device = AddDevice(&devoptab_littlefs);

    if (device == -1) {
        log_error(FileSystem, "Failed to initialize LittleFS");
    } else {
        log_info(FileSystem, "LittleFS successfully initialized (device %i)", device);
    }

    return device;
}
