// general_response.hpp
// all types of response any Dialog can return or marlin thread can sent

#pragma once

#include <cstdint>
#include <cstddef>

/**
 * @brief list of all button types
 * there is an array of texts in "client_response_texts.cpp", order and count must match!
 * !!! MAINTAIN ALPHABETICAL ORDER !!!
 */
enum class Response : uint8_t {
    _none = 0,             // none must be zero because of empty initialization of array
    Abort,                 // when used in selftest, handled automatically in PartHandler
    Abort_invalidate_test, // when used in selftest, ignored by PartHandler, must be handled in test. This behavior allows to run additional code before abort
    ABS,
    ASA,
    Back,
    Cancel,
    Change,
    Continue,
    Cooldown,
    Disable,
    Filament_removed,
    FLEX,
    FS_disable,
    HIPS,
    Ignore,
    Load,
    MMU_disable,
    Never,
    Next,
    No,
    NotNow,
    Ok,
    Pause,
    PC,
    PETG,
    PETG_NH, // PETG without heating bed
    PLA,
    PP,
    Print,
    Purge_more,
    PVB,
    Reheat,
    Restart,
    Resume,
    Retry,
    Skip,
    Slowly,
    Stop,
    Unload,
    Yes,
    Heatup,
    PA,
    PRINT,
    _last = PRINT,
};

constexpr const Response ResponseNone = Response::_none;
