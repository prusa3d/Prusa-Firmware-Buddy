#pragma once

#include "i_selftest_part.hpp"
#include "selftest_gears_config.hpp"
#include "selftest_gears_result.hpp"
#include "selftest_log.hpp"
#include <optional>

namespace selftest {

bool phase_gears(IPartHandler *&selftest_gears, const SelftestGearsConfig &config);

class SelftestGears {
    IPartHandler &state_machine;
    const SelftestGearsConfig &config;
    SelftestGearsResult &result;
    LogTimer log = 1000;

    enum class Filament { yes,
        no,
        unknown };
    Filament has_filament = Filament::unknown;
    bool need_unload = false;

    void move_gear();

public:
    SelftestGears(IPartHandler &state_machine, const SelftestGearsConfig &config, SelftestGearsResult &result);

    LoopResult state_ask_first();
    LoopResult state_get_fsensor_state();

    LoopResult state_ask_unload_init();
    LoopResult state_cycle_mark0() { return LoopResult::MarkLoop0; }
    LoopResult state_ask_unload_wait();
    LoopResult state_filament_unload_enqueue_gcode();
    LoopResult state_filament_unload_wait_finished();

    LoopResult state_release_screws_init();
    LoopResult state_release_screws();

    LoopResult state_alignment_init();
    LoopResult state_alignment();

    LoopResult state_tighten_init();
    LoopResult state_tighten();

    LoopResult state_done_init();
    LoopResult state_done();
};

} // namespace selftest
