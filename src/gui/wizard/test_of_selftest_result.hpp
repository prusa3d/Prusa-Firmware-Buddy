/**
 * @file test_of_selftest_result.hpp
 * @author Radek Vana
 * @brief Shows selftest result with dummy data
 * @date 2022-01-21
 */

#pragma once

#include "selftest_frame_result.hpp"
#include "screen.hpp"
#include "window_header.hpp"
#include "selftest_result_type.hpp"
#include <option/filament_sensor.h>

class TestResultScreen : public AddSuperWindow<screen_t> {
private:
    window_header_t header;
    SelftestFrameResult result;

    static constexpr fsm::PhaseData somethingToShow() {
        return FsmSelftestResult(0x1c).Serialize();
    }

public:
    TestResultScreen();
    void Change(fsm::BaseData data);
};
