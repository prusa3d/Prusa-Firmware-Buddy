#include "basename.h"

const char *basename(const char *path) {
    const char *last_slash = rindex(path, '/');
    if (last_slash != NULL) {
        return last_slash + 1;
    } else {
        return path;
    }
}
