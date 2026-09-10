#include "lfn.h"

#include <common/directory.hpp>
#include <cstring>
#include <dirent.h>

#include <bsod.h>

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
    Directory d { path };
    *last = '/';
    if (!d) {
        return;
    }

    struct dirent *ent;
    while ((ent = d.read())) {
        /*
         * Allow the input some flexibility - both long and short file
         * names and case insensitive (it's FAT, after all).
         */
        if ((strcasecmp(ent->d_name, fname) == 0) || (strcasecmp(ent->lfn, fname) == 0)) {
            callback(fname, ent);
            break;
        }
    }
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

void get_SFN_path(std::span<char> path) {
    search_file(path.data(), [&](char *fname, struct dirent *ent) {
        if (ent == nullptr) {
            // Fallback cases (not found / no slash): keep path intact.
            return;
        }
        // fname points into path. Skip if the SFN would not fit; the LFN (which
        // can be shorter than the SFN) is left in place.
        const size_t offset = fname - path.data();
        if (offset + strlen(ent->d_name) + 1 > path.size()) {
            return;
        }
        strcpy(fname, ent->d_name);
    });
}

void get_SFN_path_copy(const char *lfn, char *sfn_out, size_t size) {
    const char *last = rindex(lfn, '/');

    debug_assert(last);
    if (!last) {
        strlcpy(sfn_out, lfn, size);
        return;
    }

    // Copy path witout the name to sfn
    size_t len = last - lfn + 1;
    if (size <= len) {
        // This obviously won't fit, but d it anyway to have all the
        // error case do the same, it will still be valid string
        strlcpy(sfn_out, lfn, size);
        return;
    }
    strncpy(sfn_out, lfn, len);

    sfn_out[len] = '\0';

    const char *fname = last + 1;
    Directory d { sfn_out };
    if (!d) {
        strlcpy(sfn_out, lfn, size);
        return;
    }

    struct dirent *ent;
    while ((ent = d.read())) {
        /*
         * Allow the input some flexibility - both long and short file
         * names and case insensitive (it's FAT, after all).
         */
        if ((strcasecmp(ent->d_name, fname) == 0) || (strcasecmp(ent->lfn, fname) == 0)) {
            if (strlcat(sfn_out, ent->d_name, size) >= size) {
                sfn_out[0] = '\0';
            }
            break;
        }
    }

    // The lfn does not end with '/' (is a file) and we did not find it,
    // if we did not do this, we would return the path to parent dir instead,
    // which is wrong.
    if (*fname != '\0' && !ent) {
        strlcpy(sfn_out, lfn, size);
    }
}
