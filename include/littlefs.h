#pragma once

#include "lfs.h"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

/// Initialize the littlefs filesystem
lfs_t *littlefs_init();

/// Return pointer to the lfs config. NULL if not initialized yet.
struct lfs_config const *littlefs_config_get();

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)
