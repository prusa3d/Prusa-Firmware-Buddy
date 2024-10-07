#include <buddy/filesystem_semihosting.h>

#include <sys/iosupport.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <fcntl.h>

#include <buddy/filesystem.h>
#include <semihosting/semihosting.hpp>

namespace fssemihosting {
typedef struct {
    int32_t handle;
    int32_t length;
    int32_t pos;
} FIL_EX;
} // namespace fssemihosting

using fssemihosting::FIL_EX;

#define PREPARE_FIL_EX(f, fs)              \
    FIL_EX *f = static_cast<FIL_EX *>(fs); \
    if (f == NULL) {                       \
        r->_errno = EBADF;                 \
        return -1;                         \
    }

static int open_r(struct _reent *r, void *fileStruct, const char *path, int flags, __attribute__((unused)) int mode);
static ssize_t read_r(struct _reent *r, void *fileStruct, char *ptr, size_t len);
static int close_r(struct _reent *r, void *fileStruct);
static off_t seek_r(struct _reent *r, void *fileStruct, off_t pos, int dir);

static int device = -1;

static const devoptab_t devoptab_semihosting = {
    .name = "semihosting",
    .structSize = sizeof(FIL_EX),
    .open_r = open_r,
    .close_r = close_r,
    .write_r = nullptr,
    .read_r = read_r,
    .seek_r = seek_r,
    .fstat_r = nullptr,
    .stat_r = nullptr,
    .link_r = nullptr,
    .unlink_r = nullptr,
    .chdir_r = nullptr,
    .rename_r = nullptr,
    .mkdir_r = nullptr,
    .dirStateSize = 0,
    .diropen_r = nullptr,
    .dirreset_r = nullptr,
    .dirnext_r = nullptr,
    .dirclose_r = nullptr,
    .statvfs_r = nullptr,
    .ftruncate_r = nullptr,
    .fsync_r = nullptr,
    .chmod_r = nullptr,
    .fchmod_r = nullptr,
    .rmdir_r = nullptr,
    .lstat_r = nullptr,
    .utimes_r = nullptr,
};

static int open_r(struct _reent *r, void *fileStruct, const char *path, [[maybe_unused]] int flags, __attribute__((unused)) int mode) {
    PREPARE_FIL_EX(f, fileStruct);

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_semihosting.name);
    int result = semihosting::sys_open(path, semihosting::open_mode_t::OPEN_MODE_RB, strlen(path));
    if (result > 0) {
        f->pos = 0;
        f->handle = result;
        f->length = semihosting::sys_flen(f->handle);
        if (f->length == -1) {
            r->_errno = semihosting::sys_errno();
            return -1;
        }
        return 1;
    } else {
        r->_errno = semihosting::sys_errno();
        return -1;
    }
}

static ssize_t read_r(struct _reent *r, void *fileStruct, char *ptr, size_t len) {
    PREPARE_FIL_EX(f, fileStruct);

    if (!ptr) {
        r->_errno = EINVAL;
        return -1;
    }

    if (len == 0) {
        r->_errno = 0;
        return 0;
    }

    int32_t result = semihosting::sys_read(f->handle, ptr, len);
    int32_t read_bytes = len - result;

    if (result == 0) {
        // read all requested bytes and not eof
        f->pos += read_bytes;
        return len;
    } else if (result > 0 && result < static_cast<int32_t>(len)) {
        // partial success
        f->pos += read_bytes;
        return read_bytes;
    } else {
        // eof, error
        r->_errno = semihosting::sys_errno();
        return -1;
    }
}

static off_t seek_r(struct _reent *r, void *fileStruct, off_t pos, int dir) {
    PREPARE_FIL_EX(f, fileStruct);
    int32_t ofs = -1;

    if (dir == SEEK_SET) {
        ofs = pos;
    } else if (dir == SEEK_CUR) {
        if (pos == 0) {
            // No seek needed, just return the current position
            return f->pos;
        }

        ofs = f->pos + pos;
    } else if (dir == SEEK_END) {
        ofs = f->length + pos;
    }

    if (ofs < 0) {
        // Cannot seek before the beginning of a file
        r->_errno = EINVAL;
        return -1;
    }

    // It's safe to cast ofs to unsigned long when we are sure it's not negative

    if (ofs > f->length /* && (f->fil.flag & FA_WRITE) == 0 */) {
        // Unable to seek behind EOF when writing is not enabled
        r->_errno = EINVAL;
        return -1;
    }

    if (ofs == f->pos) {
        return ofs;
    }

    int result = semihosting::sys_seek(f->handle, ofs);
    if (result == 0) {
        f->pos = ofs;
        return ofs;
    } else {
        r->_errno = semihosting::sys_errno();
        return -1;
    }
}

static int close_r(struct _reent *r, void *fileStruct) {
    PREPARE_FIL_EX(f, fileStruct);

    int result = semihosting::sys_close(f->handle);
    r->_errno = semihosting::sys_errno();
    return result;
}

int filesystem_semihosting_init() {
    if (device != -1) {
        return device;
    }
    device = AddDevice(&devoptab_semihosting);
    return device;
}

void filesystem_semihosting_deinit() {
    if (device != -1) {
        RemoveDevice(devoptab_semihosting.name);
        device = -1;
    }
}

bool filesystem_semihosting_active() {
    return device != -1;
}
