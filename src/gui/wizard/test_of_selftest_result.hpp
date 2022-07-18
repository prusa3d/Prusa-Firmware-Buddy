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

class TestResult : public AddSuperWindow<screen_t> {
private:
    window_header_t header;
    SelftestFrameResult result;

    static constexpr fsm::PhaseData somethingToShow() {
        SelftestResult_t res;
        res.printFan = TestResult_t::Passed;
        res.heatBreakFan = TestResult_t::Passed;
        res.xaxis = TestResult_t::Passed;
        res.yaxis = TestResult_t::Passed;
        res.zaxis = TestResult_t::Failed;
        res.nozzle = TestResult_t::Failed;
        res.bed = TestResult_t::Failed;
        return res.Serialize();
    }

public:
    TestResult();
    void Change(fsm::BaseData data);
};
