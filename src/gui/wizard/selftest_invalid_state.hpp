/**
 * @file selftest_invalid_state.hpp
 * @brief subscreen to be shown if selftest has non existing state
 */
#pragma once

#include "selftest_frame.hpp"
#include "window_text.hpp"

class ScreenSelftestInvalidState : public SelftestFrame {
    window_text_t text;

public:
    ScreenSelftestInvalidState(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
