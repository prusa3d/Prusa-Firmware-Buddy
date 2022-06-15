#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// Starts a print.
///
/// Opens print screen (and closes all others) if GUI_WINDOW_SUPPORT is defined
void print_begin(const char *filename);

bool powerpanic_resumed_get_and_clear();

// Called once after each marlin server loop
void print_utils_loop();
#ifdef __cplusplus
}
#endif // __cplusplus
