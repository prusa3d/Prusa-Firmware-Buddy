#include "lfn.h"

#include <cassert>
#include <common/directory.hpp>
#include <cstring>
#include <dirent.h>

#include <bsod.h>
#include <utils/string_builder.hpp>

namespace {

inline void copy_string(char *dst, size_t dst_size, const char *src) {
    if (dst_size == 0) {
        return;
    }

    auto builder = StringBuilder::from_ptr(dst, dst_size);
    builder.append_string(src);
}

inline void copy_string_exact(char *dst, size_t dst_size, const char *src) {
    if (dst_size == 0) {
        return;
    }

    auto builder = StringBuilder::from_ptr(dst, dst_size);
    builder.append_string(src);
    assert(builder.is_ok());
}

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
            copy_string(lfn, lfn_size, ent->lfn);
        } else {
            copy_string(lfn, lfn_size, fname);
        }
    });
}

void get_SFN_path(char *path) {
    search_file(path, [&](char *fname, struct dirent *ent) {
        if (ent != nullptr) {
            // fname is part of the path we passed in, so we can modify it.
            copy_string_exact(fname, strlen(fname) + 1, ent->d_name);
        }
        // We are not interested in the other "fallback" cases like the LFN
        // one. We just keep path intact.
    });
}

void get_SFN_path_and_LFN(char *path, char *lfn, size_t lfn_size) {
    search_file(path, [&](char *fname, struct dirent *ent) {
        if (ent != nullptr) {
            const size_t old_fname_len = strlen(fname);
            const bool embedded_lfn = (lfn == fname + old_fname_len + 1);
            copy_string(lfn, lfn_size, ent->lfn);
            // fname is part of the path we passed in, so we can modify it.
            copy_string_exact(fname, old_fname_len + 1, ent->d_name);
            // SharedPath stores the LFN immediately behind the current path
            // terminator, so shortening the basename moves the live slot.
            if (embedded_lfn) {
                char *relocated_lfn = fname + strlen(ent->d_name) + 1;
                if (relocated_lfn != lfn) {
                    memmove(relocated_lfn, lfn, strlen(lfn) + 1);
                }
            }
        } else {
            copy_string(lfn, lfn_size, fname);
        }
    });
}

void get_SFN_path_copy(const char *lfn, char *sfn_out, size_t size) {
    const char *last = rindex(lfn, '/');

    assert(last);
    if (!last) {
        copy_string(sfn_out, size, lfn);
        return;
    }

    // Copy path witout the name to sfn
    size_t len = last - lfn + 1;
    if (size <= len) {
        // This obviously won't fit, but d it anyway to have all the
        // error case do the same, it will still be valid string
        copy_string(sfn_out, size, lfn);
        return;
    }
    memcpy(sfn_out, lfn, len);
    sfn_out[len] = '\0';

    const char *fname = last + 1;
    Directory d { sfn_out };
    if (!d) {
        copy_string(sfn_out, size, lfn);
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
        copy_string(sfn_out, size, lfn);
    }
}
