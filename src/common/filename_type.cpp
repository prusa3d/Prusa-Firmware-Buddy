#include "filename_type.hpp"
#include <dirent.h>
#include <string.h>

inline bool filename_has_ext(const char *fname, size_t len, const char *ext) {
    const size_t ext_len = strlen(ext);
    return (len >= ext_len) && (strcasecmp(fname + len - ext_len, ext) == 0);
}

inline bool filename_is_plain_gcode(const char *fname, size_t fname_len) {
    return filename_has_ext(fname, fname_len, ".g") || filename_has_ext(fname, fname_len, ".gc") || filename_has_ext(fname, fname_len, ".gco") || filename_has_ext(fname, fname_len, ".gcode");
}

bool filename_is_plain_gcode(const char *fname) {
    return filename_is_plain_gcode(fname, strlen(fname));
}

inline bool filename_is_bgcode(const char *fname, size_t fname_len) {
    return filename_has_ext(fname, fname_len, ".bgcode") || filename_has_ext(fname, fname_len, ".bgc");
}

bool filename_is_bgcode(const char *fname) {
    return filename_is_bgcode(fname, strlen(fname));
}

inline bool filename_is_printable(const char *fname, size_t fname_len) {
    return filename_is_plain_gcode(fname, fname_len) || filename_is_bgcode(fname, fname_len);
}

bool filename_is_printable(const char *fname) {
    return filename_is_printable(fname, strlen(fname));
}

inline bool filename_is_firmware(const char *fname, size_t fname_len) {
    return filename_has_ext(fname, fname_len, ".bbf");
};

bool filename_is_firmware(const char *fname) {
    return filename_is_firmware(fname, strlen(fname));
};

bool filename_is_transferrable(const char *fname) {
    const auto len = strlen(fname);
    return filename_is_printable(fname, len) || filename_is_firmware(fname, len);
}

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
