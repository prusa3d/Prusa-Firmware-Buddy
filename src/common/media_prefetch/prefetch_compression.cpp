#include "prefetch_compression.hpp"

#include <array>
#include <cctype>
#include <algorithm>

#include <stdio.h>

#ifdef UNITTESTS
    #include <catch2/catch.hpp>
    #undef assert
    #define assert(x) REQUIRE(x)
#else
    #include <assert.h>
#endif

using namespace media_prefetch;

size_t media_prefetch::compact_gcode(char *inplace_buffer) {
    const char *read_ptr = inplace_buffer;
    char *write_ptr = inplace_buffer;

    // Skip whitespaces at the beginning of the gcode, every time
    while (isspace(*read_ptr)) {
        read_ptr++;
    }

    const bool is_gx_gcode = (read_ptr[0] == 'G') && (isdigit(read_ptr[1]));

    // If the command is a G command, strip all whitespaces.
    // Other commands might contain strings and such, so we will rather not do it
    const bool strip_whitespaces = is_gx_gcode;

    // Skip comments for Gx gcodes, or if the line is just a comment
    const bool skip_comments = is_gx_gcode || (read_ptr[0] == ';');

    while (true) {
        const char ch = *read_ptr++;

        if (ch == '\0' || (ch == ';' && skip_comments)) {
            // Null or comment start - write '\0' and finish
            *write_ptr = '\0';
            return write_ptr - inplace_buffer;

        } else if (strip_whitespaces && isspace(ch)) {
            // Whitespace if we're stripping whitespaces -> skip
            continue;

        } else {
            // Otherwise copy to output
            *write_ptr++ = ch;
        }
    }
}
