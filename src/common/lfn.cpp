#include "lfn.h"

#include <cassert>
#include <cstring>
#include <dirent.h>

namespace {

template <class C>
void search_file(char *path, C &&callback) {
    /*
     * This is a bit of a hack. It sees the only place we are able to receive
     * the LFN is by iterating through a directory. So we do so and look for
     * the matching file.
     */
    char *last = rindex(path, '/');

    if (!last) {
        // This is weird. We have no slash in the path, this shouldn't happen.
        // So copy it whole just to have something.
        callback(path, nullptr);
        return;
    }

    char *fname = last + 1;
    // For the case where we don't find anything, have a result ready. If we
    // do, we overwrite it later.
    callback(fname, nullptr);

    *last = '\0';
    DIR *d = opendir(path);
    *last = '/';
    if (!d) {
        return;
    }

    struct dirent *ent;
    /*
     * Even though it doesn't look so from the signature, our readdir
     * is thread safe. The returned buffer is inside the DIR structure.
     */
    while ((ent = readdir(d))) {
        /*
         * Allow the input some flexibility - both long and short file
         * names and case insensitive (it's FAT, after all).
         */
        if ((strcasecmp(ent->d_name, fname) == 0) || (strcasecmp(ent->lfn, fname) == 0)) {
            callback(fname, ent);
            break;
        }
    }
    closedir(d);
}

} // namespace

void get_LFN(char *lfn, size_t lfn_size, char *path) {
    search_file(path, [&](char *fname, struct dirent *ent) {
        if (ent != nullptr) {
            strlcpy(lfn, ent->lfn, lfn_size);
        } else {
            strlcpy(lfn, fname, lfn_size);
        }
    });
}

void get_SFN_path(char *path) {
    search_file(path, [&](char *fname, struct dirent *ent) {
        if (ent != nullptr) {
            // fname is part of the path we passed in, so we can modify it.
            assert(strlen(fname) >= strlen(ent->d_name));
            strcpy(fname, ent->d_name);
        }
        // We are not interested in the other "fallback" cases like the LFN
        // one. We just keep path intact.
    });
}
