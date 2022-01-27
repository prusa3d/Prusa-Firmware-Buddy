#include "lfn.h"

#include <string.h>
#include <dirent.h>

void get_LFN(char *lfn, size_t lfn_size, char *path) {
    /*
     * This is a bit of a hack. It sees the only place we are able to receive
     * the LFN is by iterating through a directory. So we do so and look for
     * the matching file.
     */
    char *last = rindex(path, '/');

    if (!last) {
        // This is weird. We have no slash in the path, this shouldn't happen.
        // So copy it whole just to have something.
        strlcpy(lfn, path, lfn_size);
        return;
    }

    char *fname = last + 1;
    strlcpy(lfn, fname, lfn_size);

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
            strlcpy(lfn, ent->lfn, lfn_size);
            break;
        }
    }
    closedir(d);
}
