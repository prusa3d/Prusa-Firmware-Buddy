#pragma once

#include <marlin_events.h>

/**
 * @brief Starts a print.
 * Opens preprint preview screens and print screen (and closes all others).
 * @param filename SFN path of the file to print
 * @param skip_preview tells whether to skip parts of preview when printing is started
 */
void print_begin(const char *filename, marlin_server::PreviewSkipIfAble skip_preview = marlin_server::PreviewSkipIfAble::no);

// Called once after each marlin server loop
void print_utils_loop();

enum DeleteResult {
    Busy,
    ActiveTransfer, // do not try to delete transfer directories
    GeneralError,
    Success
};

DeleteResult remove_file(const char *path);

/// For XL, returns number of enabled dwarves.
/// For MMU, returns number of MMU slots
/// Otherwise returns 1
uint8_t get_num_of_enabled_tools();

bool is_tool_enabled(uint8_t tool);
