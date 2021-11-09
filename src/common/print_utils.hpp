#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// Starts a print.
///
/// Opens print screen (and closes all others) if GUI_WINDOW_SUPPORT is defined
void print_begin(const char *filename);

#ifdef __cplusplus
}
#endif // __cplusplus
