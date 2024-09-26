#include "path_utils.h"
#include "stat_retry.hpp"

#include <stdbool.h>
#include <sys/stat.h>

void dedup_slashes(char *filename) {
    char *write = filename;
    bool previous_slash = false;
    while (*filename) {
        char c = *filename++;
        if (c != '/' || !previous_slash) {
            *write++ = c;
        }
        previous_slash = (c == '/');
    }
    *write = '\0';
}

bool file_exists(const char *path) {
    struct stat fs;
    return stat_retry(path, &fs) == 0;
}
