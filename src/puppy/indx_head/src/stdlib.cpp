#include <sys/reent.h>
#include <sys/stat.h>

#include "hal.hpp"

extern "C" {

void abort() {
    hal::panic(indx_head::errors::FaultStatusMask::assert_failed);
}

void __assert_func(const char *, int, const char *, const char *) {
    hal::panic(indx_head::errors::FaultStatusMask::assert_failed);
}

int _close(struct _reent *, int) {
    hal::panic(indx_head::errors::FaultStatusMask::assert_failed);
}

_off_t _lseek(struct _reent *, int, _off_t, int) {
    hal::panic(indx_head::errors::FaultStatusMask::assert_failed);
}

_ssize_t _read(struct _reent *, int, void *, size_t) {
    hal::panic(indx_head::errors::FaultStatusMask::assert_failed);
}

_ssize_t _write(struct _reent *, int, const void *, size_t) {
    hal::panic(indx_head::errors::FaultStatusMask::assert_failed);
}
}
