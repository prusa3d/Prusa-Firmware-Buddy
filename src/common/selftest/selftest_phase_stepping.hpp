#pragma once

#include "i_selftest_part.hpp"
#include "selftest_phase_stepping_config.hpp"
#include "selftest_phase_stepping_result.hpp"
#include "selftest_log.hpp"
#include <optional>

namespace selftest {

bool phase_phase_stepping(IPartHandler *&selftest_phase_stepping, const SelftestPhaseSteppingConfig &config);

class SelftestPhaseStepping {
    IPartHandler &state_machine;
    const SelftestPhaseSteppingConfig &config;
    SelftestPhaseSteppingResult &result;
    LogTimer log = 1000;

    enum class Filament { yes,
        no,
        unknown };
    Filament has_filament = Filament::unknown;
    bool need_unload = false;

    void move_gear();

public:
    SelftestPhaseStepping(IPartHandler &state_machine, const SelftestPhaseSteppingConfig &config, SelftestPhaseSteppingResult &result);

    LoopResult state_calib_pick_tool();
    LoopResult state_calib_x();
    LoopResult state_calib_y();
    LoopResult state_calib_enable();
    LoopResult state_wait_until_done();

    LoopResult handle_error();
};

} // namespace selftest
