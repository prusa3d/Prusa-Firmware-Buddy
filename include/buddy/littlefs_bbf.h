#pragma once

#include "lfs.h"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

#include <stdio.h>

/// Initialize the littlefs filesystem
lfs_t *littlefs_bbf_init(FILE *bbf, uint8_t bbf_tlv_entry);

void littlefs_bbf_deinit(lfs_t *lfs);

/// Return pointer to the lfs config. NULL if not initialized yet.
struct lfs_config *littlefs_bbf_config_get();

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)
