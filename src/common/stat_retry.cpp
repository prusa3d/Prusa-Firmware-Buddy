#include "stat_retry.hpp"
#include <cerrno>
#include <sys/stat.h>

int stat_retry(const char *path, struct stat *st) {
    for (;;) {
        errno = 0;
        int result = stat(path, st);
        if (result == 0 || (errno != EAGAIN && errno != EINTR && errno != EBUSY)) {
            return result;
        }
    }
}
