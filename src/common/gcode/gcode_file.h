#pragma once
#include <stdbool.h>
#include <stdio.h>

namespace gcode {
// search this many g-code at the beginning of the file for the M862.x g-codes
static constexpr const uint32_t search_first_x_gcodes = 200;

// Search this many last bytes for "metadata" comments.
// With increasing size of the comment section, this will have to be increased either
static constexpr const size_t search_last_x_bytes = 50000;
} // namespace gcode

/// Parse comment line in given file
///
/// Reads from the file current line and parses it.
/// Example:
/// Read line: ` ; infill extrusion width = 0.40mm\n`
/// Ouptut: name = "infill extrusion width", value = "0.40mm"
bool f_gcode_get_next_comment_assignment(FILE *fp, char *name_buffer,
    int name_buffer_len,
    char *value_buffer,
    int value_buffer_len);
