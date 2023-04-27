#include "path_utils.h"

#include <stdbool.h>

void dedup_slashes(char *filename) {
    char *write = filename;
    bool previous_slash = false;
    while (*filename) {
        char c = *filename++;
        if (c != '/' || !previous_slash) {
            *write++ = c;
        }
        previous_slash = (c == '/');
    }
    *write = '\0';
}
