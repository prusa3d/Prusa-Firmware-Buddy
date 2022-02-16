#include "gcode_filename.h"

#include <string.h>

extern "C" bool filename_is_gcode(const char *fname) {
    size_t len = strlen(fname);
    static const char gcode[] = ".gcode";
    static const char gc[] = ".gc";
    static const char g[] = ".g";
    static const char gco[] = ".gco";

    if (!strcasecmp(fname + len - sizeof(gcode) + 1, gcode))
        return true;
    if (!strcasecmp(fname + len - sizeof(gc) + 1, gc))
        return true;
    if (!strcasecmp(fname + len - sizeof(g) + 1, g))
        return true;
    if (!strcasecmp(fname + len - sizeof(gco) + 1, gco))
        return true;
    return false;
}
