#pragma once

#include <marlin_vars.hpp>

namespace printer_state {

enum class DeviceState {
    Unknown,
    Idle,
    Printing,
    Paused,
    Finished,
    Stopped,
    Ready,
    Busy,
    Attention,
    Error,
};

DeviceState get_state(marlin_server::State print_state, const marlin_vars_t::FSMChange &last_fsm_state, bool ready);

bool remote_print_ready(bool preview_only);
}
