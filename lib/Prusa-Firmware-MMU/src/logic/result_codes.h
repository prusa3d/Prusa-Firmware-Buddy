/// @file result_codes.h
#pragma once
#include <stdint.h>

/// A complete set of result codes which may be a result of a high-level command/operation.
/// This header file shall be included in the printer's firmware as well as a reference,
/// therefore the error codes have been extracted to one place.
///
/// Please note that currently only LoadFilament can return something else than "OK"
enum class ResultCode : uint_fast16_t {
    OK = 0,
    Cancelled = 1
};
