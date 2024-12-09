#include <errno.h>
#include <ff.h>
#include <buddy/ffconf.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <sys/iosupport.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <buddy/filesystem.h>
#include <buddy/filesystem_fatfs.h>
#include <logging/log.hpp>
#include <marlin_client.hpp>
#include <usb_host.h>

// #define FATFS_FSTAT 1

LOG_COMPONENT_REF(FileSystem);

#define FAT_MAX_FILES         268435437
#define FAT_MAX_FILES_PER_DIR 65534

#define FF_YEAR         0b1111111000000000
#define FF_MONTH        0b0000000111100000
#define FF_DAY          0b0000000000011111
#define FF_YEAR_OFFSET  9
#define FF_MONTH_OFFSET 5
#define FF_DAY_OFFSET   0
#define FF_YEAR_MAX     (FF_YEAR >> FF_YEAR_OFFSET)

#define FF_HOUR          0b1111100000000000
#define FF_MINUTE        0b0000011111100000
#define FF_SECOND        0b0000000000011111
#define FF_HOUR_OFFSET   11
#define FF_MINUTE_OFFSET 5
#define FF_SECOND_OFFSET 0

#define FF_VALUE(value, type) ((value & type) >> type##_OFFSET)

#define FLAG_SYNC 0b00000001
#define PREPARE_FIL_EX(f, fs) \
    FIL_EX *f = fs;           \
    if (f == NULL) {          \
        r->_errno = EBADF;    \
        return -1;            \
    }

#define IS_EMPTY(s) (!s || !s[0])

namespace fatfs {
typedef struct {
    FIL fil;
    WORD flags;
#ifdef FATFS_FSTAT
    char *path; // Path is needed for fstat_r
#endif
} FIL_EX;
} // namespace fatfs

using fatfs::FIL_EX;

static int device = -1;

namespace {
extern const devoptab_t devoptab_fatfs;
}

static int get_errno(FRESULT result) {
    switch (result) {
    case FR_OK:
        return 0;
    case FR_EXIST:
        return EEXIST;
    case FR_NO_FILE:
        return ENOENT;
    case FR_NO_PATH:
        return ENOENT;
    case FR_INVALID_NAME:
        return ENOTDIR;
    case FR_WRITE_PROTECTED:
        return EROFS;
    case FR_DENIED:
        return EACCES;
    case FR_TOO_MANY_OPEN_FILES:
        return ENFILE;
    case FR_NOT_ENOUGH_CORE:
        return ENOMEM;
    case FR_DISK_ERR:
        return EIO;
    case FR_INT_ERR:
        return EFAULT; // FatFS issue, possible stack overflow
    case FR_NOT_READY:
        return EBUSY;
    case FR_INVALID_OBJECT:
        return EINVAL;
    case FR_INVALID_DRIVE:
        return ENODEV;
    case FR_NOT_ENABLED:
        return EIO;
    case FR_NO_FILESYSTEM:
        return EIO;
    case FR_MKFS_ABORTED:
        return EIO;
    case FR_TIMEOUT:
        return EAGAIN;
    case FR_LOCKED:
        return EBUSY;
    case FR_INVALID_PARAMETER:
        return EINVAL;
    default:
        return EINVAL;
    }
}

static int get_fatfs_mode(int flags) {
    int mode;

    switch (flags & O_ACCMODE) {
    case O_WRONLY:
        mode = FA_WRITE;
        break;
    case O_RDONLY:
        mode = FA_READ;
        break;
    default: // it seems we can get flags=0 on fopen(.., "w+") from our stdlib,
             // which seems incorrect, so lets make RDWR the default
    case O_RDWR:
        mode = FA_READ | FA_WRITE;
    }

    if (flags & O_APPEND) {
        mode |= FA_OPEN_APPEND;
    }

    if (flags & O_CREAT) {
        mode |= FA_OPEN_ALWAYS;
    }

    if (flags & O_EXCL) {
        mode |= FA_CREATE_NEW;
    }

    if (flags & O_TRUNC) {
        mode |= FA_CREATE_ALWAYS;
    }

    return mode;
}

static time_t get_posix_time(DWORD fdate, DWORD ftime) {
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));

    tm.tm_year = FF_VALUE(fdate, FF_YEAR) + 80; // FAT year origin from 1980, tm_year origin from 1900
    tm.tm_mon = FF_VALUE(fdate, FF_MONTH) - 1; // FAT count months from 1, tm from 0 :-O
    tm.tm_mday = FF_VALUE(fdate, FF_DAY);
    tm.tm_hour = FF_VALUE(ftime, FF_HOUR);
    tm.tm_min = FF_VALUE(ftime, FF_MINUTE);
    tm.tm_sec = FF_VALUE(ftime, FF_SECOND);

    return mktime(&tm);
}

#if FF_USE_CHMOD && !FF_FS_READONLY
static FILINFO get_fatfs_time(
    const struct timeval last_access,
    const struct timeval modification) {

    FILINFO finfo;
    struct tm *tm;
    time_t time;

    memset(&finfo, 0, sizeof(FILINFO));

    // Preffered is modification time before last access
    time = modification.tv_sec == 0 ? modification.tv_sec : last_access.tv_sec;

    if (time == 0) {
        // we need current time in case of provided empty time
        // TODO: Get current time
    }

    tm = gmtime(&time);

    tm->tm_year -= 80; // FAT year origin from 1980, tm_year origin from 1900
    if (tm->tm_year < 0) {
        // FAT doesn't support year before 1980
        tm->tm_year = 0;
    } else if (tm->tm_year > FF_YEAR_MAX) {
        // More than FAT max value, set the last possible date
        tm->tm_year = FF_YEAR_MAX;
        tm->tm_mon = 12;
        tm->tm_mday = 31;
        tm->tm_hour = 23;
        tm->tm_min = 59;
        tm->tm_sec = 59;
    }

    finfo.fdate = tm->tm_year << FF_YEAR_OFFSET;
    finfo.fdate |= tm->tm_mon << FF_MONTH_OFFSET;
    finfo.fdate |= tm->tm_mday << FF_DAY_OFFSET;

    finfo.ftime = tm->tm_hour << FF_HOUR_OFFSET;
    finfo.ftime |= tm->tm_min << FF_MINUTE_OFFSET;
    finfo.ftime |= tm->tm_sec << FF_SECOND_OFFSET;

    return finfo;
}
#endif

static int open_r(struct _reent *r, void *fileStruct, const char *path, int flags, __attribute__((unused)) int mode) {
    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    FRESULT result;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_fatfs.name);

    int ff_mode = get_fatfs_mode(flags);

    if (!ff_mode) {
        r->_errno = EINVAL;
        return -1;
    }

#ifdef FATFS_FSTAT
    size_t len = strlen(path) + 1; // +1 for the termination char
    f->path = malloc(len);
    if (f->path) {
        strlcpy(f->path, path, len);
    }
#endif

    result = f_open(&(f->fil), path, ff_mode);
    r->_errno = get_errno(result);

    if (result != FR_OK) {
        if (result == FR_NO_FILESYSTEM) {
            // Displays exact message that file system on usb disk is not supported
            marlin_client::set_warning(WarningType::USBDriveUnsupportedFileSystem);
            usb_host::disable_media();
        }
        return -1;
    }

    f->flags = 0;
    if (flags & O_SYNC) {
        f->flags |= FLAG_SYNC;
    }

    return result == FR_OK;
}

static int close_r(struct _reent *r, void *fileStruct) {
    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    FRESULT result;
#ifdef FATFS_FSTAT
    if (f->path) {
        free(f->path);
    }
#endif
    result = f_close(&(f->fil));
    r->_errno = get_errno(result);

    return result == FR_OK ? 0 : -1;
}

static int fsync_r(struct _reent *r, void *fileStruct) {
    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    FRESULT result;

    result = f_sync(&(f->fil));
    r->_errno = get_errno(result);

    if (result != FR_OK) {
        return -1;
    }

    return 0;
}

static ssize_t write_r(struct _reent *r, void *fileStruct, const char *ptr, size_t len) {
    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    FRESULT result;
    UINT bytes_written;

    if (!ptr) {
        r->_errno = EINVAL;
        return -1;
    }

    if (len == 0) {
        r->_errno = 0;
        return 0;
    }

    result = f_write(&(f->fil), ptr, len, &bytes_written);
    r->_errno = get_errno(result);

    if (result != FR_OK) {
        return -1;
    }

    if (f->flags & FLAG_SYNC) {
        fsync_r(r, fileStruct);
    }

    return bytes_written;
}

static ssize_t read_r(struct _reent *r, void *fileStruct, char *ptr, size_t len) {
    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    FRESULT result;
    UINT bytes_read;

    if (!ptr) {
        r->_errno = EINVAL;
        return -1;
    }

    if (len == 0) {
        r->_errno = 0;
        return 0;
    }

    result = f_read(&(f->fil), ptr, len, &bytes_read);
    r->_errno = get_errno(result);

    if (result != FR_OK) {
        return -1;
    }

    return bytes_read;
}

static off_t seek_r(struct _reent *r, void *fileStruct, off_t pos, int dir) {
    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    FRESULT result;
    const unsigned long size = f_size(&(f->fil));
    long ofs = -1;

    if (dir == SEEK_SET) {
        ofs = pos;
    } else if (dir == SEEK_CUR) {
        if (pos == 0) {
            // No seek needed, just return the current position
            return f_tell(&(f->fil));
        }

        ofs = f_tell(&(f->fil)) + pos;
    } else if (dir == SEEK_END) {
        ofs = size + pos;
    }

    if (ofs < 0) {
        // Cannot seek befor the beginning of a file
        r->_errno = EINVAL;
        return -1;
    }

    // It's safe to cast ofs to unsigned long when we are sure it's not negative

    if ((unsigned long)ofs > size && (f->fil.flag & FA_WRITE) == 0) {
        // Unable to seek behind EOF when writing is not enabled
        r->_errno = EINVAL;
        return -1;
    }

    result = f_lseek(&(f->fil), ofs);
    r->_errno = get_errno(result);

    if (result != FR_OK) {
        return -1;
    }

    if (f_tell(&(f->fil)) != (unsigned long)ofs) {
        // FatFS lseek can be successful even when not reached required position
        // It shouldn't happen after we have already checked for EOF,
        // but it's better to be safe
        r->_errno = EIO;
        return -1;
    }

    return ofs;
}

static int stat_r(struct _reent *r, const char *path, struct stat *st) {
    FRESULT result;
    FILINFO finfo;
    memset(&finfo, 0, sizeof(FILINFO));
    memset(st, 0, sizeof(struct stat));

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_fatfs.name);

    result = f_stat(path, &finfo);
    if (result != FR_OK) {
        r->_errno = get_errno(result);
        return -1;
    }

    st->st_mode = S_IRUSR | S_IRGRP | S_IROTH; // Everybody can read

    if (!(finfo.fattrib & AM_RDO)) {
        // If not read only, everybody can write
        st->st_mode |= IS_IWALL;
    }
    if (finfo.fattrib & AM_HID) {
        // Ignore "hidden" flag
    }
    if (finfo.fattrib & AM_SYS) {
        // Ignore "system" flag
    }
    if (finfo.fattrib & AM_ARC) {
        // Ignore "archive" flag
    }
    if (finfo.fattrib & AM_DIR) {
        st->st_mode |= S_IFDIR;
    } else {
        st->st_mode |= S_IFREG;
    }

    st->st_dev = device;
    st->st_size = finfo.fsize;
    st->st_blksize = FF_MAX_SS;
    st->st_blocks = 1;
    st->st_mtime = get_posix_time(finfo.fdate, finfo.ftime);

    return 0;
}

static int fstat_r(struct _reent *r, __attribute__((unused)) void *fileStruct, __attribute__((unused)) struct stat *st) {
#ifdef FATFS_FSTAT
    PREPARE_FIL_EX(f, fileStruct);
    if (f->path) {
        int res = stat_r(r, f->path, st);
        if (res == 0 && f->fil.flag & FA_READ) {
            // File is opened for write only, remove read flag
            st->st_mode &= ~IS_IRALL;
        }
        return res;
    }
    r->_errno = ENOENT;
#else
    r->_errno = ENOTSUP;
#endif
    return -1;
}

static int link_r(struct _reent *r, __attribute__((unused)) const char *existing, __attribute__((unused)) const char *newLink) {
    // Links are not supported on FAT
    r->_errno = ENOTSUP;
    return -1;
}

static int unlink_r(struct _reent *r, const char *path) {
    FRESULT result;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_fatfs.name);

    result = f_unlink(path);
    r->_errno = get_errno(result);

    if (result != FR_OK) {
        return -1;
    }

    return 0;
}

static int chdir_r(struct _reent *r, const char *path) {
    FRESULT result;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_fatfs.name);

    result = f_chdir(path);
    r->_errno = get_errno(result);

    if (result != FR_OK) {
        return -1;
    }

    return 0;
}

static int rename_r(struct _reent *r, const char *oldName, const char *newName) {
    FRESULT result;

    if (IS_EMPTY(oldName) || IS_EMPTY(newName)) {
        r->_errno = EINVAL;
        return -1;
    }

    oldName = process_path(oldName, devoptab_fatfs.name);
    newName = process_path(newName, devoptab_fatfs.name);

    result = f_rename(oldName, newName);
    r->_errno = get_errno(result);

    if (result != FR_OK) {
        return -1;
    }

    return 0;
}

static int chmod_r(struct _reent *r, __attribute__((unused)) const char *path, __attribute__((unused)) mode_t mode) {
#if !FF_USE_CHMOD || FF_FS_READONLY
    r->_errno = ENOTSUP;
    return -1;
#else
    FRESULT result;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_fatfs.name);

    BYTE attr = (mode & IS_IWALL) ? 0 : AM_RDO; // Read only when no write enabled
    // Ignoring Archive, System and Hidden attributes

    result = f_chmod(path, attr, AM_RDO);
    r->_errno = get_errno(result);

    if (result != FR_OK) {
        return -1;
    }

    return 0;
#endif
}

static int fchmod_r(struct _reent *r, __attribute__((unused)) void *fileStruct, __attribute__((unused)) mode_t mode) {
#ifdef FATFS_FSTAT
    PREPARE_FIL_EX(f, fileStruct);
    if (f->path) {
        return chmod_r(r, f->path, mode);
    }
#endif
    r->_errno = ENOTSUP;
    return -1;
}

static int mkdir_r(struct _reent *r, const char *path, int mode) {
    FRESULT result;
    int errno;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_fatfs.name);

    result = f_mkdir(path);
    errno = get_errno(result);
    r->_errno = errno;

    if (result != FR_OK) {
        return -1;
    }

    if (!(mode & IS_IWALL)) {
        // Write not enabled, make new dir readonly
        chmod_r(r, path, 0);
    }

    // Set errno again because chmod can fail if FatFS chmod is not enabled,
    // that would change r->_errno value.
    // In that case we will just ignore chmod error and return mkdir result.
    r->_errno = errno;

    return 0;
}

static DIR_ITER *diropen_r(struct _reent *r, DIR_ITER *dirState, const char *path) {
    FRESULT result;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return NULL;
    }

    path = process_path(path, devoptab_fatfs.name);

    result = f_opendir(static_cast<DIR *>(dirState->dirStruct), path);
    r->_errno = get_errno(result);

    if (result != FR_OK) {
        return NULL;
    }

    return dirState;
}

static int dirreset_r(struct _reent *r, DIR_ITER *dirState) {
    FRESULT result;

    result = f_rewinddir(static_cast<DIR *>(dirState->dirStruct));
    r->_errno = get_errno(result);

    return result == FR_OK ? 0 : -1;
}

static int dirnext_r(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat) {
    FRESULT result;
    FILINFO fno;

    if (filename == NULL || filestat == NULL) {
        r->_errno = EINVAL;
        return -1;
    }
    do {
        result = f_readdir(static_cast<DIR *>(dirState->dirStruct), &fno);
        r->_errno = get_errno(result);

        if (result != FR_OK) {
            return -1;
        }
        if (fno.fname[0] == 0) {
            break;
        }
    } while (fno.fattrib & (AM_SYS | AM_HID));

    if (!fno.fname[0]) {
        // Empty filename means end of directory
        r->_errno = ENOENT; // has to be set at the end
        return -1;
    }

    if (fno.altname[0] != 0) {
        strlcpy(filename, fno.altname, NAME_MAX);
        uint8_t len = strnlen(filename, NAME_MAX);
        // filename is 256 chars long (NAME_MAX) so 13 + 105 chars is shorter, and it will fit inside
        strlcpy(filename + len + 1, fno.fname, NAME_MAX - len - 1);
    } else {
        strlcpy(filename, fno.fname, NAME_MAX);
        // put zero on first byte of lfn to disable lfn for this file/folder
        uint8_t len = strnlen(filename, NAME_MAX);
        filename[len + 1] = 0;
    }

    filestat->st_mode = fno.fattrib & AM_DIR ? S_IFDIR : S_IFREG;
    filestat->st_mtime = get_posix_time(fno.fdate, fno.ftime);

    return 0;
}

static int dirclose_r(struct _reent *r, DIR_ITER *dirState) {
    FRESULT result;

    result = f_closedir(static_cast<DIR *>(dirState->dirStruct));
    r->_errno = get_errno(result);

    return 0;
}

static int statvfs_r(struct _reent *r, const char *path, struct statvfs *buf) {
    FRESULT result;
    FATFS *ff;
    DWORD free_clst;

    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return -1;
    }

    path = process_path(path, devoptab_fatfs.name);

    result = f_getfree(path, &free_clst, &ff);
    if (result != FR_OK) {
        r->_errno = get_errno(result);
        return -1;
    }

    memset(buf, 0, sizeof(struct statvfs));

    buf->f_frsize = ff->csize;
#if FF_MAX_SS != FF_MIN_SS
    buf->f_bsize = ff->ssize;
#else
    buf->f_bsize = FF_MAX_SS;
#endif
    buf->f_bfree = free_clst;
    buf->f_bavail = buf->f_bfree;
    buf->f_files = 0; // TODO: Count all inodes
    buf->f_ffree = FAT_MAX_FILES - buf->f_files;
    buf->f_favail = buf->f_ffree;
    buf->f_fsid = (device & 0xFFFF) | (((uint32_t)(ff->pdrv)) << 16); // 16b for filesystems, 16b for driver per filesystem
    buf->f_flag = ST_NOSUID;
#ifdef _USE_LFN
    buf->f_namemax = _MAX_LFN;
#else
    buf->f_namemax = 12;
#endif

    return 0;
}

static int ftruncate_r(struct _reent *r, void *fileStruct, off_t len) {
    PREPARE_FIL_EX(f, static_cast<FIL_EX *>(fileStruct));
    FRESULT result;
    int pos;

    pos = f_tell(&(f->fil));

    // We need to seek to the desired len
    if (seek_r(r, fileStruct, len, SEEK_SET) != len) {
        r->_errno = EIO;
        return -1;
    }

    // Truncate the file size to the current position
    result = f_truncate(&(f->fil));
    r->_errno = get_errno(result);

    if (result != FR_OK) {
        return -1;
    }

    // Seek back
    result = static_cast<FRESULT>(seek_r(r, fileStruct, pos, SEEK_SET));
    r->_errno = get_errno(result);

    return 0;
}

static int rmdir_r(struct _reent *r, const char *path) {
    return unlink_r(r, path);
}

static int lstat_r(struct _reent *r, const char *file, struct stat *st) {
    // FAT doesn't support links, just return stat
    return stat_r(r, file, st);
}

static int utimes_r(struct _reent *r, __attribute__((unused)) const char *filename, __attribute__((unused)) const struct timeval times[2]) {
#if !FF_USE_CHMOD || FF_FS_READONLY
    r->_errno = EPERM;
    return -1;
#else
    FRESULT result;
    FILINFO finfo;

    finfo = get_fatfs_time(times[0], times[1]);
    result = f_utime(filename, &finfo);
    r->_errno = get_errno(result);

    if (result != FR_OK) {
        return -1;
    }

    return 0;
#endif
}

namespace {
const devoptab_t devoptab_fatfs = {
    .name = "usb",
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
    .dirStateSize = sizeof(DIR),
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
}

FIL *filesystem_fastfs_get_underlying_struct(FILE *file) {
    auto fno = fileno(file);
    auto handle = __get_handle(fno);

    if ((int)handle->device != device) {
        log_error(FileSystem, "File %i is not backed by FatFS", fno);
        return nullptr;
    }

    FIL_EX *fil = static_cast<FIL_EX *>(handle->fileStruct);
    return &fil->fil;
}

int filesystem_fatfs_init() {
    if (device != -1) {
        // Already initialized
        return device;
    }

    device = AddDevice(&devoptab_fatfs);

    if (device == -1) {
        log_error(FileSystem, "Failed to initialize FatFS");
    } else {
        log_info(FileSystem, "FatFS successfully initialized (device %i)", device);
    }

    return device;
}
