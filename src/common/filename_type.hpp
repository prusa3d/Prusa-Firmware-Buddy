#pragma once

#include <stdbool.h>

struct dirent;

bool filename_is_firmware(const char *fname);

const char *file_type_by_ext(const char *fname);

const char *file_type(const dirent *ent);

/**
 * \brief Does the filename look like printable file (bgcode or plain gcode)
 *
 * Checks the extensions and decides if it is most probably a gcode file.
 */
bool filename_is_printable(const char *fname);

/**
 * @brief Filename is plaintext gcode
 */
bool filename_is_plain_gcode(const char *fname);

/**
 * @brief Filename is binary gcode
 */
bool filename_is_bgcode(const char *fname);

/// Returns whether the filename is supported by connect transfers
bool filename_is_transferrable(const char *filename);
