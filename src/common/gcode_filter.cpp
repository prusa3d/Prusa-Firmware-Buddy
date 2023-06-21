#include "gcode_filter.hpp"

char *GCodeFilter::nextGcode(State *state) {
    for (;;) {
        const char nextByte = getByte(state);
        if (*state == Eof) {
            if (offset != 0 && !wait_new_line) {
                // process the read data
                return prepareGcode();
            }
            // no bytes read return NULL
            return NULL;
        }
        if (*state != Ok) {
            return NULL;
        }

        if (nextByte == '\n') {
            if (wait_new_line) {
                wait_new_line = false;
            } else if (offset != 0) { // Don't process empty lines
                char *gcode = prepareGcode();
                if (gcode != NULL) {
                    return gcode;
                }
            }
            continue; // Skip the new line char
        } else if (wait_new_line) {
            continue; // Skip until next line
        }

        if (nextByte == '\r') {
            continue; // Skip CR
        }

        if (nextByte == ' ' && offset == 0) {
            continue; // Skip leading spaces
        }

        if (nextByte == ';') {
            // Found a comment, skip until next line
            wait_new_line = true;
            if (offset > 0) {
                // Process the part before the comment
                char *gcode = prepareGcode();
                if (gcode == NULL) {
                    continue; // Nothing to return, continue
                }
                return gcode;
            }
            continue;
        }

        // Add new char to the buffer
        buffer[offset++] = nextByte;

        if (offset == buffer_size - 1) {
            // Too long line, return what we have and continue to the next line
            wait_new_line = true;
            char *gcode = prepareGcode();
            if (gcode == NULL) {
                continue; // Nothing to return, continue
            }
            return gcode;
        }
    }
    // Should be never reached
    *state = Error;
    return NULL;
}

char *GCodeFilter::prepareGcode() {
    // Skip trailing spaces and CR
    for (; offset > 0 && buffer[offset - 1] == ' '; offset--)
        ;

    if (offset == 0) {
        // The buffer is empty, no G-Code to return
        return NULL;
    }

    // Buffer doesn't contain termination char,
    // we need to insert it before returning G-Code
    buffer[offset] = '\0';

    // Clear the offset before next line
    offset = 0;

    return buffer;
}

void GCodeFilter::reset() {
    offset = 0;
    wait_new_line = false;
}
