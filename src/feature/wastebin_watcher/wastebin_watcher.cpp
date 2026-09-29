/// @file
#include <feature/wastebin_watcher/wastebin_watcher.hpp>

#include <algorithm>

#include <odometer.hpp>
#include <config_store/store_instance.hpp>
#include <gcode/gcode_info.hpp>
#include <marlin_server.hpp>
#include <common/marlin_vars.hpp>
#include <warning_type.hpp>
#include <client_response.hpp>
#include <fsm/wastebin_emptying_phases.hpp>
#include <inc/MarlinConfig.h>
#include <mapi/parking.hpp>
#include <mapi/motion.hpp>
#include <mapi/feedrates/standard_feedrates.hpp>
#include <nozzle_cleaner.hpp>
#include <Marlin/src/module/motion.h>
#include <Marlin/src/module/temperature.h>
#include <module/planner.h>

WastebinWatcher &WastebinWatcher::instance() {
    static WastebinWatcher watcher;
    return watcher;
}

void WastebinWatcher::account_ejected_pellet() {
    const uint32_t before = Odometer_s::instance().get_nozzle_cleaner_pellets();
    Odometer_s::instance().add_nozzle_cleaner_pellet();
    ++pellets_this_print_;

    // React once, the moment the count crosses capacity during a print (unless monitoring was
    // disabled for this print via Ignore).
    const uint32_t cap = capacity();
    if (!marlin_server::is_printing() || ignored_for_print_ || before >= cap || before + 1 < cap) {
        return;
    }

    if (config_store().nozzle_cleaner_autopause_on_full.get()) {
        // Auto-pause: park + hold the print right here (we're on the marlin thread, directly after
        // the eject) until the user answers the prompt. No asynchronous gcode injection.
        pause_to_empty(/*full=*/true);
    } else {
        // Auto-pause off: informational only (the print keeps running; the user cannot safely empty
        // the bin mid-print, so this variant has just a dismiss button, no Done/Ignore actions).
        marlin_server::set_warning(WarningType::NozzleCleanerFullInfo);
    }
}

void WastebinWatcher::pause_to_empty(bool full) {
    const std::optional<PhysicalToolIndex> tool = PhysicalToolIndex::currently_selected_opt();
    // Deliberately narrower than is_printing(): the print preview (overfill prompt) also runs
    // M1986, and there is nothing to return to there.
    const bool printing = marlin_server::is_printing_state(marlin_vars().print_state);
    const xyz_pos_t resume_pos = current_position.xyz();
    const float resume_e = current_position.e;
    // Cleaner is outside the MBL mesh; save/restore Z in the machine frame (like G750), not native.
    const float resume_machine_z = to_machine_pos(current_position).z;

    const int16_t resume_temp = tool ? Hotend::for_tool(*tool).nozzle_target_temp() : 0;

    const auto filament = FilamentType::for_current_tool_heuristic();

    // Scores over Printing and PrintPreview - all the entry points get the same screens.
    marlin_server::FSM_Holder fsm { PhaseWastebinEmptying::parking };

    if (printing && tool.has_value()) {
        // Drop the nozzle target so it doesn't cook/ooze during the (possibly long) wait; restored
        // and reheated over the cleaner before returning to the print.
        Hotend::for_tool(*tool).set_nozzle_target_temp(0);
        // Retract to the standard pre-park distance so the nozzle does not ooze while parked, then
        // park clear of the cleaner. retract_to() only moves the delta to the target, so it won't
        // over-retract (pull the filament out of the gears) if the print already had it retracted.
        // We block the gcode stream here (= effective pause); print_pause() + wait would deadlock
        // this gcode processor (BFW-8821).
        mapi::retract_to(STANDARD_RETRACT_LENGTH, buddy::standard_feedrates::extruder(buddy::standard_feedrates::Extruder::retract, filament));
    }
    mapi::home_if_needed_and_park(mapi::get_parking_position(mapi::ParkPosition::empty_wastebin));

    // full=true: the bin hit capacity, so Ignore is offered to stop checking it for this print.
    const auto prompt = full ? PhaseWastebinEmptying::prompt_full : PhaseWastebinEmptying::prompt_empty;
    fsm.change(prompt);

    switch (marlin_server::wait_for_response(prompt)) {
    case Response::Done:
        mark_emptied(); // bin emptied -> reset the counter
        break;
    case Response::Ignore:
        set_ignored_for_print(true); // stop checking the bin for the rest of this print
        break;
    default:
        break;
    }

    if (!printing) {
        return; // stay parked
    }

    fsm.change(PhaseWastebinEmptying::raising_bed);
    // Park over in the nozzle cleaner so that reheating blob lands in the wastebin
    mapi::park(mapi::get_parking_position(mapi::ParkPosition::purge));
    // Move the most of the Z travel over the wastebin
    constexpr float return_clearance = 5;
    const MachinePosXYZE over_bin = current_machine_position().with_z(std::max(resume_machine_z, planner.max_printed_z + return_clearance));
    line_to_machine_pos(over_bin, NOZZLE_PARK_Z_FEEDRATE, { .ignore_e_factor = false });
    // Let the bed finish rising before announcing the heat-up, so the screen matches what moves.
    planner.synchronize();

    if (tool.has_value()) {
        fsm.change(PhaseWastebinEmptying::heating);
        // Reheat over the cleaner so heat-up ooze drips into the bin, not onto the print.
        Hotend::for_tool(*tool).set_nozzle_target_temp(resume_temp);
        thermalManager.wait_for_hotend(*tool, { .no_wait_for_cooling = false });

        fsm.change(PhaseWastebinEmptying::purging);
        // The nozzle stays picked and residual heating is oozing the filament, the nozzle needs to be primed to return to printing
        // It ends autoretracted, so on success declare E restored and let the deretract below no-op
        // On failure E is untouched and the deretract restores the pre-park retract as before
        if (!thermalManager.tooColdToExtrude(*tool)
            && nozzle_cleaner::load_and_execute(nozzle_cleaner::Sequence::purge_clean)) {
            sync_e_position_to(resume_e);
        }
    }

    fsm.change(PhaseWastebinEmptying::returning);
    // Traverse XY back (park routes around the cleaner) at that clearance height so we never drag
    // over the print, then drop to the saved machine Z and restore E.
    mapi::park(mapi::ParkingPosition { .x = resume_pos.x, .y = resume_pos.y });
    line_to_machine_pos(current_machine_position().with_z(resume_machine_z), NOZZLE_PARK_Z_FEEDRATE, { .ignore_e_factor = false });
    planner.synchronize();
    mapi::extruder_move(resume_e - current_position.e, buddy::standard_feedrates::extruder(buddy::standard_feedrates::Extruder::deretract, filament));
}

bool WastebinWatcher::print_will_overfill() const {
    // Pre-print projection: will the whole upcoming print overflow the bin (one pellet per toolchange)?
    const auto total = GCodeInfo::getInstance().get_total_toolchanges();
    if (!total.has_value()) {
        return false;
    }
    return Odometer_s::instance().get_nozzle_cleaner_pellets() + *total > capacity();
}

void WastebinWatcher::reset_print_progress() {
    pellets_this_print_ = 0;
}

void WastebinWatcher::set_ignored_for_print(bool ignored) {
    ignored_for_print_ = ignored;
}

void WastebinWatcher::mark_emptied() {
    Odometer_s::instance().reset_nozzle_cleaner_pellets();
}

uint32_t WastebinWatcher::capacity() const {
    return config_store().nozzle_cleaner_extended_capacity.get()
        ? NOZZLE_CLEANER_WASTEBIN_CAPACITY_EXTENDED
        : NOZZLE_CLEANER_WASTEBIN_CAPACITY_BASIC;
}

uint32_t WastebinWatcher::fill_level() const {
    return Odometer_s::instance().get_nozzle_cleaner_pellets();
}

std::optional<uint32_t> WastebinWatcher::expected_remaining_pellets() const {
    // One pellet is ejected per toolchange, so the gcode's total toolchange count is the number of
    // pellets this print will eject; subtract the ones already ejected to get what is still ahead.
    const auto total = GCodeInfo::getInstance().get_total_toolchanges();
    if (!total.has_value()) {
        return std::nullopt;
    }
    const uint32_t done = pellets_this_print_;
    return *total > done ? *total - done : 0;
}
