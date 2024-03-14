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

const char *dirent_lfn(const struct dirent *ent) {
#ifdef UNITTESTS
    return ent->d_name;
#else
    if (ent->lfn != nullptr) {
        return ent->lfn;
    } else {
        // Fatfs without long file name...
        return ent->d_name;
    }
#endif
}
