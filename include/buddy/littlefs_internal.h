#pragma once

#include "lfs.h"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

/// Initialize the littlefs filesystem
lfs_t *littlefs_internal_init();

/// Return pointer to the lfs config. NULL if not initialized yet.
struct lfs_config *littlefs_internal_config_get();

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)
