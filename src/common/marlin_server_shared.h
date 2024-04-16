#pragma once

#include "cmsis_os.h" // for osThreadId
#include <limits>
#include <time.h>
#include "marlin_server_types/marlin_server_state.h"
#include <utils/utility_extensions.hpp>

namespace marlin_server {

inline constexpr uint32_t TIME_TO_END_INVALID = std::numeric_limits<uint32_t>::max();
inline constexpr time_t TIMESTAMP_INVALID = std::numeric_limits<time_t>::max();

inline constexpr uint8_t CURRENT_TOOL = std::numeric_limits<uint8_t>::max();

inline bool is_abort_state(State st) {
    return ftrstd::to_underlying(st) >= ftrstd::to_underlying(State::Aborting_Begin) && ftrstd::to_underlying(st) <= ftrstd::to_underlying(State::Aborted);
}

extern osThreadId server_task; // task of marlin server

} // namespace marlin_server
