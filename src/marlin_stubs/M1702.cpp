#include <option/has_coldpull.h>
#include <option/has_mmu2.h>
#include <option/has_toolchanger.h>

#include <M70X.hpp>
#include <fs_autoload_autolock.hpp>

#include <module/temperature.h>

#include <common/cold_pull.hpp>
#include <client_fsm_types.h>
#include <client_response.hpp>
#include <common/marlin_server.hpp>
#include <common/RAII.hpp>

#if HAS_TOOLCHANGER()
    #include <window_tool_action_box.hpp>
#endif

#if HAS_MMU2()
    #include <feature/prusa/MMU2/mmu2_mk4.h>
#endif

#include <optional>

namespace PrusaGcodeSuite {

#if HAS_COLDPULL()

namespace {

    constexpr const uint16_t HOTEND_COLD_TEMP { 36 };
    constexpr const uint16_t HOTEND_PULL_TEMP { 80 };
    constexpr const uint16_t HOTEND_END_TEMP { 85 };

    constexpr const millis_t TIMEOUT_DISABLED { 0 };
    constexpr const millis_t COOLING_TIMEOUT_MILLIS { 1000 * 60 * 15 };
    constexpr const millis_t PROGRESS_MILLIS { 500 }; // in ms

    #if HAS_TOOLCHANGER()
    uint8_t selected_tool { 0 };
    #endif

    #if HAS_MMU2()
    bool was_mmu_enabled { false };
    #endif

    bool was_success { false };

    using marlin_server::wait_for_response;

    template <typename CMP, typename CBK>
    Response wait_while_with_progress(
        const PhasesColdPull phase, const millis_t timeout, CMP &&compare, CBK &&progress) {

        millis_t now = millis();

        const millis_t deadline = now + timeout;
        millis_t progress_timeout { now };

        while (compare()) {
            if (Response response = marlin_server::get_response_from_phase(phase); response != Response::_none) {
                return response;
            }

            now = millis();

            if (timeout && ELAPSED(now, deadline)) {
                break;
            }
            if (ELAPSED(now, progress_timeout)) {
                progress(deadline - now);
                progress_timeout = now + PROGRESS_MILLIS;
            }
            idle(true);
        }
        return Response::_none;
    }

    PhasesColdPull info() {
        switch (wait_for_response(PhasesColdPull::introduction)) {
        case Response::Stop:
            return PhasesColdPull::cleanup;
        case Response::Continue:
    #if HAS_TOOLCHANGER()
            return prusa_toolchanger.is_toolchanger_enabled() ? PhasesColdPull::select_tool : PhasesColdPull::unload_ptfe;
    #elif HAS_MMU2()
            return MMU2::mmu2.Enabled() ? PhasesColdPull::unload_ptfe : PhasesColdPull::prepare_filament;
    #else
            return PhasesColdPull::prepare_filament;
    #endif
        default:
            bsod("Invalid phase encountered.");
        }
    }

    #if HAS_TOOLCHANGER()
    PhasesColdPull select_tool() {
        selected_tool = PrusaToolChanger::MARLIN_NO_TOOL_PICKED;
        const auto r { wait_for_response(PhasesColdPull::select_tool) };
        switch (r) {
        case Response::Tool1:
        case Response::Tool2:
        case Response::Tool3:
        case Response::Tool4:
        case Response::Tool5:
            selected_tool = ftrstd::to_underlying(r) - ftrstd::to_underlying(Response::Tool1);
            return PhasesColdPull::pick_tool;
        case Response::Continue:
            selected_tool = active_extruder;
            return PhasesColdPull::pick_tool;
        default:
            bsod("Invalid phase encountered.");
        }
    }

    PhasesColdPull pick_tool() {
        if (selected_tool == PrusaToolChanger::MARLIN_NO_TOOL_PICKED && active_extruder == PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
            return PhasesColdPull::introduction;
        }
        tool_change(selected_tool, tool_return_t::no_return, tool_change_lift_t::full_lift, 1);
        if (active_extruder == PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
            return PhasesColdPull::introduction;
        }
        return PhasesColdPull::unload_ptfe;
    }
    #endif

    #if HAS_MMU2()

    PhasesColdPull stop_mmu() {
        if (MMU2::mmu2.Enabled() == true) {

            // Two reasons for the show_time:
            // - show the screen for at least 1.5 seconds (it's almost instant and unreadable otherwise)
            // - must wait a bit while the new MMU state propagates everywhere
            constexpr const millis_t MINIMAL_SHOW_TIME { 1500 };
            const millis_t deadline = millis() + MINIMAL_SHOW_TIME;

            auto progress = [](auto) {}; // intentionally empty
            auto mmu_on_timed = [&]() {
                return MMU2::mmu2.Enabled() == true || millis() < deadline;
            };

            filament_gcodes::mmu_off();

            switch (wait_while_with_progress(PhasesColdPull::stop_mmu, 0, mmu_on_timed, progress)) {
            case Response::Abort:
                return PhasesColdPull::cleanup;
            case Response::_none:
                break;
            default:
                bsod("Invalid phase encountered.");
            }

            idle(true); // Still do one event-loop in case the MMU stop took too long.
        }

        return PhasesColdPull::blank_load;
    }

    PhasesColdPull cleanup() {
        if (was_mmu_enabled && MMU2::mmu2.Enabled() == false) {

            auto progress = [](auto) {}; // intentionally empty
            auto mmu_off = []() {
                return MMU2::mmu2.Enabled() == false;
            };

            filament_gcodes::mmu_on();

            idle(true);
            wait_while_with_progress(PhasesColdPull::cleanup, 0, mmu_off, progress);
        }

        return was_success ? PhasesColdPull::pull_done : PhasesColdPull::finish;
    }

    #endif

    PhasesColdPull unload_ptfe() {
        switch (wait_for_response(PhasesColdPull::unload_ptfe)) {
        case Response::Unload:
            return PhasesColdPull::blank_unload;
        case Response::Continue:
            return PhasesColdPull::load_ptfe;
        case Response::Abort:
            return PhasesColdPull::cleanup;
        default:
            bsod("Invalid phase encountered.");
        }
    }

    PhasesColdPull load_ptfe() {
        switch (wait_for_response(PhasesColdPull::load_ptfe)) {
        case Response::Load:
    #if HAS_MMU2()
            return was_mmu_enabled ? PhasesColdPull::stop_mmu : PhasesColdPull::blank_load;
    #else
            return PhasesColdPull::blank_load;
    #endif
        case Response::Continue:
            return PhasesColdPull::cool_down;
        case Response::Abort:
            return PhasesColdPull::cleanup;
        default:
            bsod("Invalid phase encountered.");
        }
    }

    PhasesColdPull prepare_filament() {
        switch (wait_for_response(PhasesColdPull::prepare_filament)) {
        case Response::Unload:
            return PhasesColdPull::blank_unload;
        case Response::Load:
            return PhasesColdPull::blank_load;
        case Response::Continue:
            return PhasesColdPull::cool_down;
        case Response::Abort:
            return PhasesColdPull::cleanup;
        default:
            bsod("Invalid phase encountered.");
        }
    }

    PhasesColdPull blank_unload() {
        filament_gcodes::M702_no_parser(
            std::nullopt,
            Z_AXIS_UNLOAD_POS,
            RetAndCool_t::Return,
            active_extruder,
    #if HAS_MMU2()
            !MMU2::mmu2.Enabled() // MUST be false when MMU is enabled otherwise unload wont do full length
    #else
            true
    #endif
        );
        planner.resume_queuing(); // HACK for planner.quick_stop(); in Pause::check_user_stop()

    #if HAS_TOOLCHANGER() || HAS_MMU2()
        return PhasesColdPull::load_ptfe;
    #else
        return PhasesColdPull::prepare_filament;
    #endif
    }

    PhasesColdPull blank_load() {

        filament_gcodes::M701_no_parser(
            PresetFilamentType::PLA,
            std::nullopt,
            Z_AXIS_LOAD_POS,
            RetAndCool_t::Return,
            active_extruder,
    #if HAS_MMU2()
            MMU2::FILAMENT_UNKNOWN,
    #else
            -1,
    #endif
            std::nullopt,
            filament_gcodes::ResumePrint_t::No);
        planner.resume_queuing(); // HACK for planner.quick_stop(); in Pause::check_user_stop()

        switch (PreheatStatus::ConsumeResult()) {
        case PreheatStatus::Result::DoneHasFilament:
            return PhasesColdPull::cool_down;
        default:
    #if HAS_TOOLCHANGER() || HAS_MMU2()
            return PhasesColdPull::load_ptfe;
    #else
            return PhasesColdPull::prepare_filament;
    #endif
        }
    }

    PhasesColdPull cool_down() {
        thermalManager.disable_hotend(); // cool down without target to avoid PID handling target temp
        marlin_server::set_temp_to_display(HOTEND_COLD_TEMP, active_extruder); // still show target temperature

        auto too_hot = []() {
            return static_cast<uint16_t>(Temperature::degHotend(active_extruder)) > HOTEND_COLD_TEMP;
        };

        auto progress = [](const millis_t time_left) {
            cold_pull::TemperatureProgressData data { { 0 } };
            data.percent = static_cast<uint8_t>(
                100.0f * HOTEND_COLD_TEMP / Temperature::degHotend(active_extruder));
            data.time_sec = time_left / 1000;
            marlin_server::fsm_change(PhasesColdPull::cool_down, data.fsm_data);
        };

        const auto fan_speed_stored = Temperature::fan_speed[0];
        thermalManager.set_fan_speed(0, 240);

        switch (wait_while_with_progress(PhasesColdPull::cool_down, COOLING_TIMEOUT_MILLIS, too_hot, progress)) {
        case Response::Abort:
            return PhasesColdPull::cleanup;
        case Response::_none:
            break;
        default:
            bsod("Invalid phase encountered.");
        }
        thermalManager.set_fan_speed(0, fan_speed_stored);
        return PhasesColdPull::heat_up;
    }

    PhasesColdPull heat_up() {
        thermalManager.setTargetHotend(HOTEND_PULL_TEMP, active_extruder);
        marlin_server::set_temp_to_display(HOTEND_PULL_TEMP, active_extruder);

        auto too_cold = []() {
            return static_cast<uint16_t>(Temperature::degHotend(active_extruder)) < Temperature::degTargetHotend(active_extruder);
        };

        auto progress = [](millis_t) {
            cold_pull::TemperatureProgressData data { { 0 } };
            data.percent = static_cast<uint8_t>(
                100.0f * Temperature::degHotend(active_extruder) / Temperature::degTargetHotend(active_extruder));
            marlin_server::fsm_change(PhasesColdPull::heat_up, data.fsm_data);
        };

        switch (wait_while_with_progress(PhasesColdPull::heat_up, TIMEOUT_DISABLED, too_cold, progress)) {
        case Response::Abort:
            return PhasesColdPull::cleanup;
        case Response::_none:
            break;
        default:
            bsod("Invalid phase encountered.");
        }

        return PhasesColdPull::automatic_pull;
    }

    PhasesColdPull automatic_pull() {
        thermalManager.setTargetHotend(HOTEND_END_TEMP, active_extruder);
        marlin_server::set_temp_to_display(HOTEND_END_TEMP, active_extruder);

        {
    #if ENABLED(PREVENT_COLD_EXTRUSION)
            AutoRestore cold_extrude_restore(thermalManager.allow_cold_extrude, true);
    #endif
            plan_move_by(50, 0, 0, 0, -std::min(300, EXTRUDE_MAXLENGTH));
            planner.synchronize();

            // mark filament unloaded
            config_store().set_filament_type(active_extruder, FilamentType::none);
            filament_gcodes::M70X_process_user_response(PreheatStatus::Result::DoneNoFilament, active_extruder);
        }
        return PhasesColdPull::manual_pull;
    }

    PhasesColdPull manual_pull() {
        thermalManager.disable_hotend();
        marlin_server::set_temp_to_display(0, active_extruder);

        was_success = true;

        switch (wait_for_response(PhasesColdPull::manual_pull)) {
        case Response::Continue:
            break;
        default:
            bsod("Invalid phase encountered.");
        }

        return PhasesColdPull::cleanup;
    }

    PhasesColdPull pull_done() {
        switch (wait_for_response(PhasesColdPull::pull_done)) {
        case Response::Finish:
            break;
        default:
            bsod("Invalid phase encountered.");
        }

        return PhasesColdPull::finish;
    }

    PhasesColdPull get_next_phase(const PhasesColdPull phase) {
        switch (phase) {
        case PhasesColdPull::introduction:
            return info();
    #if HAS_TOOLCHANGER()
        case PhasesColdPull::select_tool:
            return select_tool();
        case PhasesColdPull::pick_tool:
            return pick_tool();
    #endif
    #if HAS_MMU2()
        case PhasesColdPull::stop_mmu:
            return stop_mmu();
    #endif
    #if HAS_TOOLCHANGER() || HAS_MMU2()
        case PhasesColdPull::unload_ptfe:
            return unload_ptfe();
        case PhasesColdPull::load_ptfe:
            return load_ptfe();
    #endif
        case PhasesColdPull::prepare_filament:
            return prepare_filament();
        case PhasesColdPull::blank_unload:
            return blank_unload();
        case PhasesColdPull::blank_load:
            return blank_load();
        case PhasesColdPull::cool_down:
            return cool_down();
        case PhasesColdPull::heat_up:
            return heat_up();
        case PhasesColdPull::automatic_pull:
            return automatic_pull();
        case PhasesColdPull::manual_pull:
            return manual_pull();
        case PhasesColdPull::cleanup:
    #if HAS_MMU2()
            return cleanup();
    #else
            return PhasesColdPull::finish;
    #endif
        case PhasesColdPull::pull_done:
            return pull_done();
        case PhasesColdPull::finish:
            break;
        }
        bsod("Invalid phase encountered.");
    }

} // namespace

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M1702: Prevent filament autoload during whole ColdPull workflow
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M1702
 *
 */

void M1702() {
    // Prevent filament autoload during whole ColdPull workflow.
    FS_AutoloadAutolock lock;

    #if HAS_MMU2()
    was_mmu_enabled = MMU2::mmu2.Enabled();
    #endif

    was_success = false;

    PhasesColdPull phase = PhasesColdPull::introduction;
    marlin_server::FSM_Holder holder { PhasesColdPull::introduction };
    do {
        phase = get_next_phase(phase);
        marlin_server::fsm_change(phase, {});
    } while (phase != PhasesColdPull::finish);

    // Clean up
    thermalManager.zero_fan_speeds();
    thermalManager.disable_all_heaters();
    marlin_server::set_temp_to_display(0, active_extruder);
}

#else

void M1702() {}

#endif

} // namespace PrusaGcodeSuite

/** @}*/
