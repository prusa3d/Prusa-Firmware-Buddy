// general_response.hpp
// all types of response any Dialog can return or marlin thread can sent

#pragma once

#include <cstdint>
#include <cstddef>

// list of all button types
// there is an array of texts in "client_response_texts.cpp"
// order and count must match!
enum class Response : uint8_t {
    _none = 0, //none must be zero because of empty initialization of array
    Abort,
    ABS,
    ASA,
    Back,
    Cancel,
    Change,
    Continue,
    Cooldown,
    Filament_removed,
    FLEX,
    HIPS,
    Ignore,
    Load,
    Next,
    No,
    Ok,
    PC,
    PETG,
    PLA,
    PP,
    Purge_more,
    PVB,
    Reheat,
    Retry,
    Skip,
    Stop,
    Unload,
    Yes,
    _last = Yes
};
