#include "filepath_operation.h"

const char *basename_b(const char *path) {
    const char *last_slash = rindex(path, '/');
    if (last_slash != NULL) {
        return last_slash + 1;
    } else {
        return path;
    }
}

void dirname(char *path) {
    char *last_slash = rindex(path, '/');
    if (last_slash == NULL) {
        return;
    }
    *last_slash = '\0';
}
