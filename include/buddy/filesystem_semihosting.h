#pragma once

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

int filesystem_semihosting_init();
void filesystem_semihosting_deinit();
bool filesystem_semihosting_active();

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)
