#include "auto_retract.hpp"

#include <marlin_vars.hpp>
#include <config_store/store_instance.hpp>
#include <feature/ramming/standard_ramming_sequence.hpp>
#include <module/planner.h>
#include <raii/auto_restore.hpp>
#include <feature/gcode_exception/gcode_exception.hpp>
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include <logging/log.hpp>
#include <feature/print_status_message/print_status_message_guard.hpp>
#include <marlin_server.hpp>
#include <feature/prusa/e-stall_detector.h>
#include <mapi/motion.hpp>
#include <mapi/parking.hpp>
#include <mapi/feedrates/standard_feedrates.hpp>
#include <gcode/temperature/M104_M109.hpp>
#include <module/raii/include/raii/scope_guard.hpp>
#include <feature/safety_timer/safety_timer.hpp>

#include <option/has_mmu2.h>
#include <option/has_switchable_auto_retract.h>
#include <bsod/bsod.h>
#if HAS_MMU2()
    #include <Marlin/src/feature/prusa/MMU2/mmu2_mk4.h>
#endif

LOG_COMPONENT_REF(MarlinServer);

using namespace buddy;
using namespace auto_retract_detail;

// If the STANDARD_RETRACT_LENGTH would get over full_retract_distance, it would start being considered rammed for cold unload
// We don't want that to happen, for cold unload, a proper ramming should be done
static_assert(!AutoRetract::supports_cold_unload || STANDARD_RETRACT_LENGTH < AutoRetract::full_retract_distance);

AutoRetract &buddy::auto_retract() {
    static AutoRetract instance;
    return instance;
}

AutoRetract::AutoRetract() {
    for (auto tool : PhysicalToolIndex::all()) {
        const auto dist = config_store().get_filament_retracted_distance(tool);
        retracted_hotends_bitset_.set(tool.to_raw(), dist.value_or(0.0f) > 0.0f);
        known_hotends_bitset_.set(tool.to_raw(), dist.has_value());
    }
}
bool AutoRetract::will_deretract(ToolVariant tool) const {
    return match(
        tool, //
        [this](PhysicalToolIndex physical_tool) -> bool { return retracted_hotends_bitset_.test(physical_tool.to_raw()); }, //
        [](NoTool) -> bool { return false; } //
    );
}

bool AutoRetract::is_fully_retracted(ToolVariant tool) const {
    const auto dist = retracted_distance(tool);
    return dist.has_value() && dist.value() >= full_retract_distance;
}

std::optional<float> AutoRetract::retracted_distance(ToolVariant tool) const {
    return match(
        tool, //
        [](PhysicalToolIndex physical_tool) -> std::optional<float> { return config_store().get_filament_retracted_distance(physical_tool); }, //
        [](NoTool) -> std::optional<float> { return std::nullopt; } //
    );
}
bool AutoRetract::can_cold_unload([[maybe_unused]] PhysicalToolIndex physical_tool) const {
    if constexpr (supports_cold_unload) {
        return is_fully_retracted(physical_tool);
    } else {
        return false;
    }
}

void AutoRetract::set_retracted_distance(PhysicalToolIndex tool, std::optional<float> dist) {
    if (!dist.has_value() && !known_hotends_bitset_.test(tool.to_raw())) {
        // To reduce mutex locking
        return;
    }
    known_hotends_bitset_.set(tool.to_raw(), dist.has_value());
    retracted_hotends_bitset_.set(tool.to_raw(), dist.value_or(0.0f) > 0.0f);
    config_store().set_filament_retracted_distance(tool, dist);
}

void AutoRetract::maybe_retract_from_nozzle(const RetractFromNozzleParams &params) {
    if (gcode_exceptions().is_unwinding()) {
        return;
    }

    const auto maybe_tool = VirtualToolIndex::currently_selected();
    if (std::holds_alternative<NoTool>(maybe_tool)) {
        return;
    }
    const auto virtual_tool = std::get<VirtualToolIndex>(maybe_tool);
    const auto physical_tool = virtual_tool.to_physical();

    // Is already retracted -> exit
    if (is_fully_retracted(physical_tool)) {
        return;
    }

#if HAS_SWITCHABLE_AUTO_RETRACT()
    // Do not auto retract when disabled globally
    if (!config_store().auto_retract_enabled.get()) {
        return;
    }
#endif

    const auto filament_parameters = FilamentType::for_tool_heuristic(virtual_tool).parameters();

    // Do not auto retract flexible filaments, they might get tangled in the extruder (BFW-6953)
    if (filament_parameters.is_flexible) {
        return;
    }

    PrintStatusMessageGuard psm_guard;
    psm_guard.update<PrintStatusMessage::Type::auto_retracting>({});

    Hotend &hotend = Hotend::for_tool(physical_tool);

    safety_timer().reset_restore_nonblocking();

    // restore actual target temperature after autorectract
    const auto original_temp = hotend.nozzle_target_temp();
    ScopeGuard temp_restorer([&]() {
        hotend.set_nozzle_target_temp(original_temp);
    });

    // heat up the nozzle (especially important for INDX where nozzle can cool down before autoretract is finished)
    hotend.set_nozzle_target_temp(std::max(original_temp, filament_parameters.nozzle_temperature));
    thermalManager.wait_for_hotend(physical_tool, { .no_wait_for_cooling = true, .early_return_temperature = filament_parameters.nozzle_temperature });

#if HAS_WASTEBIN()
    if (params.park_over_wastebin) {
        mapi::home_if_needed_and_park(mapi::get_parking_position(mapi::ParkPosition::purge).without_z_move());
    }
#endif

    // Finish all pending movements so that the progress reporting is nice
    planner.synchronize();

    // We might be retracted a bit, deretract to make sure the ramming sequence runs proper
    maybe_deretract_to_nozzle();

    const auto &sequence = standard_ramming_sequence(StandardRammingSequence::auto_retract, virtual_tool);
    {
        // No estall detection during the ramming; we may do so too fast sometimes
        // to the point where the motor skips, but we don't care, as it doesn't
        // damage the print.
        BlockEStallDetection estall_blocker;

        struct {
            uint32_t start_time;
            float progress_coef;
            const ProgressCallback &progress_callback;
        } progress_data {
            ticks_ms(),
            100.0f / sequence.duration_estimate_ms(),
            params.progress_callback
        };

        Subscriber subscriber(marlin_server::idle_publisher, [&] {
            const float progress_0_100 = std::min((ticks_ms() - progress_data.start_time) * progress_data.progress_coef, 100.0f);
            psm_guard.update<PrintStatusMessage::Type::auto_retracting>({ progress_0_100 });
            if (progress_data.progress_callback) {
                progress_data.progress_callback(progress_0_100);
            }
        });
        sequence.execute();
    }

    debug_assert(!supports_cold_unload || sequence.retracted_distance() >= full_retract_distance);
    set_retracted_distance(physical_tool, sequence.retracted_distance());
}

void AutoRetract::maybe_deretract_to_nozzle() {
    // Prevent deretract nesting
    if (is_checking_deretract_) {
        return;
    }
    AutoRestore ar(is_checking_deretract_, true);

    const auto physical_tool_opt = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::currently_selected());
    if (!physical_tool_opt.has_value()) {
        return;
    }

    const auto physical_tool = *physical_tool_opt;

    // Is not retracted -> exit
    if (!will_deretract(physical_tool)) {
        return;
    }

    if (thermalManager.tooColdToExtrude(physical_tool) || gcode_exceptions().is_unwinding()) {
        if (!DEBUGGING(DRYRUN)) {
            // With dry run this spams logs and overflows the RTT buffers
            log_error(MarlinServer, "auto_retract: Cannot perform deretract");
        }
        return;
    }

    // Necessary before we sample the positions
    planner.synchronize();

    const auto orig_e_position = current_position.e;
    const auto orig_e_planner_position = planner.get_axis_position_mm(E_AXIS_N(active_extruder));

    {
        // No estall detection during the ramming; we may do so too fast sometimes
        // to the point where the motor skips, but we don't care, as it doesn't
        // damage the print.
        BlockEStallDetection estall_blocker;
        mapi::extruder_move(retracted_distance(physical_tool).value_or(0.0f), standard_feedrates::current_extruder(standard_feedrates::Extruder::deretract));
        planner.synchronize();
    }

    // "Fake" original extruder position - we are interrupting various movements by this function,
    // firmware gets very confused if the current position changes while it is planning a move
    current_position.e = orig_e_position;

    // We need to fake the planner position separately, because some rogue functions update the current_position BEFORE issuing the raw move
    // So precondition that current_position == planner.position_float does not hold
    planner.set_e_position_mm(orig_e_planner_position);

    set_retracted_distance(physical_tool, 0.0f);
}

void AutoRetract::ensure_retracted_no_ramming(float purge_length) {
    const auto virtual_tool_opt = stdext::get_optional<VirtualToolIndex>(VirtualToolIndex::currently_selected());
    if (!virtual_tool_opt.has_value()) {
        return;
    }

    const auto virtual_tool = *virtual_tool_opt;
    const auto physical_tool = virtual_tool.to_physical();

    debug_assert(purge_length >= 0.0f); // no sense having negative purge length
    if (this->retracted_distance(physical_tool) >= full_retract_distance) {
        return; // should not do anything when already retracted more than standard distance
    }

    planner.synchronize();
    const auto temp_before = thermalManager.degTargetHotend(physical_tool);
    const M109Flags flags_pre = {
        .target_temp = FilamentType::for_tool_heuristic(virtual_tool).parameters().nozzle_temperature,
    };
    M109_no_parser(physical_tool, flags_pre);

    {
        const auto filament = FilamentType::for_current_tool_heuristic();

        BlockEStallDetection estall_blocker;
        // Purge a little
        if (purge_length > 0.f) {
            mapi::extruder_move(purge_length, buddy::standard_feedrates::extruder(buddy::standard_feedrates::Extruder::advanced_pause_purge, filament));
            planner.synchronize();
        }
        // Retract
        const float retracted_distance = this->retracted_distance(physical_tool).value_or(0.f);
        const float retract_amount = full_retract_distance - retracted_distance;
        mapi::extruder_move(-retract_amount, buddy::standard_feedrates::extruder(buddy::standard_feedrates::Extruder::retract, filament));
        planner.synchronize();
        set_retracted_distance(physical_tool, full_retract_distance);
    }

    // Reach back to original temp
    const M109Flags flags_post = {
        .target_temp = temp_before,
        .wait_heat_or_cool = true,
        .autotemp = true, // Use fans to cool
    };
    M109_no_parser(physical_tool, flags_post);
}
