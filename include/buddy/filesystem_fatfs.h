#pragma once

#include <stdio.h>
#include "ff.h"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

int filesystem_fatfs_init();

FIL *filesystem_fastfs_get_underlying_struct(FILE *file);

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)
