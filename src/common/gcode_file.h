#pragma once
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

int f_gcode_thumb_open(FILE *fp, FILE *gcode_fp);
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

#ifdef __cplusplus
}
#endif //__cplusplus
