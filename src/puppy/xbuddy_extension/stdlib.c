/// @file
#include <sys/reent.h>

#include "hal.h"

// For now, let's compile with stdlib and suffer these functions.
// Buildsystem is not yet ready for -nostdlib

int _close_r(struct _reent *, int) {
    hal_panic();
}

_off_t _lseek_r(struct _reent *, int, _off_t, int) {
    hal_panic();
}

_ssize_t _read_r(struct _reent *, int, void *, size_t) {
    hal_panic();
}

_ssize_t _write_r(struct _reent *, int, const void *, size_t) {
    hal_panic();
}

int _kill_r(struct _reent *, int, int) {
    hal_panic();
}

int _getpid_r(struct _reent *) {
    hal_panic();
}
