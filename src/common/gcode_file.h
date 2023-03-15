#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <span>
#include <optional>
#include "gcode_thumb_decoder.h"

int f_gcode_thumb_open(GCodeThumbDecoder *gd, FILE *fp);
int f_gcode_thumb_close(FILE *fp);

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

/// Parse line with several items delimited by a separator
std::optional<std::span<char>> f_gcode_iter_items(std::span<char> &buffer, char separatorb);

/// Search this many last bytes for "metadata" comments.
/// With increasing size of the comment section, this will have to be increased either
const long f_gcode_search_last_x_bytes = 25000;
