// general_response.hpp
// all types of response any Dialog can return or marlin thread can sent

#pragma once

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <array>
#include <string_view>

/** list of all button types */
enum class Response : uint8_t {
    _none = 0, // none must be zero because of empty initialization of array
    Abort, // when used in selftest, handled automatically in PartHandler
    Abort_invalidate_test, // when used in selftest, ignored by PartHandler, must be handled in test. This behavior allows to run additional code before abort
    ABS,
    Adjust,
    All,
    ASA,
    Back,
    Cancel,
    Change,
    Continue,
    Cooldown,
    Disable,
    Done,
    Filament,
    Filament_removed,
    Finish,
    FLEX,
    FS_disable,
    NozzleType_Normal,
    NozzleType_HighFlow,
    HIPS,
    Ignore,
    Left,
    Load,
    MMU_disable,
    Never,
    Next,
    No,
    NotNow,
    NozzleDiameter_04,
    NozzleDiameter_06,
    Ok,
    Pause,
    PC,
    PETG,
    PLA,
    PP,
    Print,
    HotendType_Stock,
    HotendType_StockWithSock,
    HotendType_E3DRevo,
    Purge_more,
    PVB,
    Quit,
    Reheat,
    Replace,
    Remove,
    Restart,
    Resume,
    Retry,
    Right,
    Skip,
    Slowly,
    SpoolJoin,
    Stop,
    Unload,
    Yes,
    Heatup,
    PA,
    Postpone5Days,
    PRINT,
    _count,
    _last = _count - 1,
};

inline constexpr Response ResponseNone = Response::_none;

template <size_t N>
constexpr size_t cnt_filled_responses(const std::array<Response, N> &resps) {
    return (std::find(resps.begin(), resps.end(), Response::_none)) - resps.begin();
}

template <size_t N>
constexpr size_t get_response_idx(const std::array<Response, N> &resps, Response resp) {
    return std::distance(resps.begin(), std::find(resps.begin(), resps.end(), resp));
}

Response from_str(std::string_view str);
const char *to_str(const Response response);
