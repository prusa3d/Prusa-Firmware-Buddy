#pragma once

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

// Root filesystem implements only reading root directory and returns
// list of directories representing mounted filesystems
int filesystem_root_init();

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)
