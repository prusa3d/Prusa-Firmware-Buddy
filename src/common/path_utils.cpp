#include "path_utils.h"
#include "stat_retry.hpp"
#include <sys/stat.h>

#include <buddy/filename_defs.hpp>

#include <array>
#include <stdbool.h>
#include <errno.h>

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
}

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

bool make_dirs(std::string_view path) {
    std::array<char, filename_defs::path_buffer_size> dir;
    size_t path_size = path.rfind('/') + 1;
    if (path_size >= dir.size() || !path.starts_with('/')) {
        return false;
    }
    // 1 to skip the first slash
    size_t pos = 1;
    while ((pos = path.find('/', pos + 1)) != path.npos) {
        strlcpy(dir.data(), path.data(), pos + 1);
        if (mkdir(dir.data(), 0777) != 0 && errno != EEXIST) {
            return false;
        }
    }

    return true;
}
