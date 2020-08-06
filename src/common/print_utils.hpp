#pragma once

/// Starts a print.
///
/// Opens print screen (and closes all others) if GUI_WINDOW_SUPPORT is defined
void print_begin(const char *filename);
