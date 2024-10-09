#pragma once

#include <cstdint>

#include <error_codes.hpp>

/// Map the error code from the enum to an actual error code for the printer (can differ based on the configured printer type)
uint16_t map_error_code(ErrCode code);
