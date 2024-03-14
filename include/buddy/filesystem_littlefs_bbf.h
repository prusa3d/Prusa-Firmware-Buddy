#pragma once

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

#include <stdio.h>

int filesystem_littlefs_bbf_init(FILE *file, uint8_t entry);
void filesystem_littlefs_bbf_deinit();

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)
