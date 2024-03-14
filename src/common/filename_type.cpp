#include "filename_type.hpp"
#include <dirent.h>

#include <string.h>

bool filename_has_ext(const char *fname, const char *ext) {
    const size_t len = strlen(fname);
    const size_t ext_len = strlen(ext);

    return (len >= ext_len) && (strcasecmp(fname + len - ext_len, ext) == 0);
}

bool filename_is_plain_gcode(const char *fname) {
    return filename_has_ext(fname, ".gcode") || filename_has_ext(fname, ".gc") || filename_has_ext(fname, ".g") || filename_has_ext(fname, ".gco");
}

bool filename_is_bgcode(const char *fname) {
    return filename_has_ext(fname, ".bgcode") || filename_has_ext(fname, ".bgc");
}

bool filename_is_printable(const char *fname) {
    return filename_is_plain_gcode(fname) || filename_is_bgcode(fname);
}

bool filename_is_firmware(const char *fname) {
    return filename_has_ext(fname, ".bbf");
};

const char *file_type_by_ext(const char *fname) {
    if (filename_is_printable(fname)) {
        return "PRINT_FILE";
    } else if (filename_is_firmware(fname)) {
        return "FIRMWARE";
    } else {
        return "FILE";
    }
}

const char *file_type(const dirent *ent) {
    if (ent->d_type == DT_DIR) {
        return "FOLDER";
    } else {
        return file_type_by_ext(ent->d_name);
    }
}
