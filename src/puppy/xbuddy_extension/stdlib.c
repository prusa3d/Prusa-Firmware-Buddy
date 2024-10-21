/// @file
#include <sys/reent.h>

// For now, let's compile with stdlib and suffer these functions.
// Buildsystem is not yet ready for -nostdlib

int _close_r(struct _reent *, int) {
    for (;;)
        ;
}

_off_t _lseek_r(struct _reent *, int, _off_t, int) {
    for (;;)
        ;
}

_ssize_t _read_r(struct _reent *, int, void *, size_t) {
    for (;;)
        ;
}

_ssize_t _write_r(struct _reent *, int, const void *, size_t) {
    for (;;)
        ;
}

int _kill_r(struct _reent *, int, int) {
    for (;;)
        ;
}

int _getpid_r(struct _reent *) {
    for (;;)
        ;
}
