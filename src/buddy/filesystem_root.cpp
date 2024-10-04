#include <errno.h>
#include <string.h>
#include <sys/iosupport.h>
#include <sys/syslimits.h>
#include <buddy/filesystem.h>
#include <buddy/filesystem_root.h>

#define DIR_INDEX ((DIR *)dirState->dirStruct)->index

int device = -1;

typedef struct {
    int index;
} DIR;

static DIR_ITER *diropen_r(struct _reent *r, DIR_ITER *dirState, const char *path) {
    if (IS_EMPTY(path)) {
        r->_errno = EINVAL;
        return NULL;
    }

    // Accepts only path to the root (can contain only slashes)
    do {
        if (*path != '/') {
            r->_errno = ENOTDIR;
            return NULL;
        }
    } while (*(++path) != '\0');

    // Use dirStruct only as a counter of current position
    // Skip stdin, stdout and stderr
    DIR_INDEX = 3;
    r->_errno = 0;

    return dirState;
}

static int dirreset_r(struct _reent *r, DIR_ITER *dirState) {
    // Skip stdin, stdout and stderr
    DIR_INDEX = 3;
    r->_errno = 0;
    return 0;
}

static int dirnext_r(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat) {
    if (DIR_INDEX < 3) {
        return -1;
    }

    for (;; DIR_INDEX++) {
        if (DIR_INDEX >= STD_MAX) {
            return -1;
        }
        if (DIR_INDEX != device && strcmp(devoptab_list[DIR_INDEX]->name, "stdnull")) {
            break;
        }
        // Skip this device and stdnull
    }

    strlcpy(filename, devoptab_list[DIR_INDEX]->name, NAME_MAX);
    filestat->st_mode = S_IFDIR;

    DIR_INDEX++;

    r->_errno = 0;
    return 0;
}

static int dirclose_r(struct _reent *r, __attribute__((unused)) DIR_ITER *dirState) {
    r->_errno = 0;
    return 0;
}

static const devoptab_t devoptab_root = {
    .name = "",
    .structSize = 0,
    .open_r = NULL,
    .close_r = NULL,
    .write_r = NULL,
    .read_r = NULL,
    .seek_r = NULL,
    .fstat_r = NULL,
    .stat_r = NULL,
    .link_r = NULL,
    .unlink_r = NULL,
    .chdir_r = NULL,
    .rename_r = NULL,
    .mkdir_r = NULL,
    .dirStateSize = sizeof(DIR),
    .diropen_r = diropen_r,
    .dirreset_r = dirreset_r,
    .dirnext_r = dirnext_r,
    .dirclose_r = dirclose_r,
    .statvfs_r = NULL,
    .ftruncate_r = NULL,
    .fsync_r = NULL,
    .chmod_r = NULL,
    .fchmod_r = NULL,
    .rmdir_r = NULL,
    .lstat_r = NULL,
    .utimes_r = NULL,
};

int filesystem_root_init() {
    if (device == -1) {
        device = AddDevice(&devoptab_root);
    }
    return device;
}
