#include "gcode_filename.hpp"

#include <string.h>

bool filename_has_ext(const char *fname, const char *ext) {
    const size_t len = strlen(fname);
    const size_t ext_len = strlen(ext);

    return (len >= ext_len) && (strcasecmp(fname + len - ext_len, ext) == 0);
}

bool filename_is_gcode(const char *fname) {
    return filename_has_ext(fname, ".gcode") || filename_has_ext(fname, ".gc") || filename_has_ext(fname, ".g") || filename_has_ext(fname, ".gco");
}
