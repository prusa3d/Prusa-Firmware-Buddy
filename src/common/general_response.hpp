// general_response.hpp
// all types of response any Dialog can return or marlin thread can sent

#pragma once

#include <cstdint>
#include <cstddef>

//list of all button types
enum class Response : uint8_t {
    _none = 0, //none must be zero because of empty initialization of array
    Abort,
    Back,
    Cancel,
    Continue,
    Filament_removed,
    Ignore,
    No,
    Ok,
    Purge_more,
    Reheat,
    Retry,
    Stop,
    Yes,
    _last = Yes
};
