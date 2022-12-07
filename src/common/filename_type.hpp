#pragma once

#include <stdbool.h>

struct dirent;

bool filename_is_firmware(const char *fname);

const char *file_type_by_ext(const char *fname);

const char *file_type(const dirent *ent);

bool filename_has_ext(const char *fname, const char *ext);

/**
 * \brief Does the filename look like a gcode?
 *
 * Checks the extensions and decides if it is most probably a gcode file.
 */
bool filename_is_gcode(const char *fname);
