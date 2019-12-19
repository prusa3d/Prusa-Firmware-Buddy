#pragma once

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Starts a print.
///
/// This does nothing to GUI and presenting the proper screen
/// is the caller's responsibility.
void print_begin(const char *filename);

#ifdef __cplusplus
}
#endif //__cplusplus
