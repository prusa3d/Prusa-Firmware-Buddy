#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compute printer hash.
//
// * Does _not_ set the terminating '\0' (if you need it, set it yourself).
// * Allows setting the number of characters output.
extern void printerHash(char *str_buffer, size_t size);

#ifdef __cplusplus
}
#endif
