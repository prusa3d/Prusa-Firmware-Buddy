#pragma once

#include <marlin_events.h>

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
