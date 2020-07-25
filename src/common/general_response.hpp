// general_response.hpp
// all types of response any Dialog can return or marlin thread can sent

#pragma once

#include <cstdint>
#include <cstddef>

//list of all button types
enum class Response : uint8_t {
    _none = 0, //none must be zero because of empty initialization of array
    Yes,
    No,
    Continue,
    Ok,
    Back,
    Retry,
    Stop,
    Purge_more,
    Reheat,
    Filament_removed,
    _last = Filament_removed
};
