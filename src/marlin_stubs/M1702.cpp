#include <option/has_coldpull.h>

#include <M70X.hpp>
#include <fs_autoload_autolock.hpp>

#include <module/temperature.h>

#include <common/cold_pull.hpp>
#include <common/client_fsm_types.h>
#include <common/client_response.hpp>
#include <common/marlin_server.hpp>
#include <common/RAII.hpp>

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

    Response wait_for_response(const PhasesColdPull phase) {
        for (;;) {
            if (Response response = marlin_server::get_response_from_phase(phase); response != Response::_none) {
                return response;
            }
            idle(true);
        }
    }

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
            return PhasesColdPull::finish;
        case Response::Continue:
            return PhasesColdPull::prepare_filament;
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
            return PhasesColdPull::finish;
        default:
            bsod("Invalid phase encountered.");
        }
    }

    PhasesColdPull blank_unload() {
        filament_gcodes::M702_no_parser(
            std::nullopt, Z_AXIS_UNLOAD_POS, RetAndCool_t::Return, active_extruder, true);
        planner.resume_queuing(); // HACK for planner.quick_stop(); in Pause::check_user_stop()
        return PhasesColdPull::prepare_filament;
    }

    PhasesColdPull blank_load() {
        filament_gcodes::M701_no_parser(
            filament::Type::PLA,
            std::nullopt,
            Z_AXIS_LOAD_POS,
            RetAndCool_t::Return,
            active_extruder,
            -1,
            std::nullopt,
            filament_gcodes::ResumePrint_t::No);
        planner.resume_queuing(); // HACK for planner.quick_stop(); in Pause::check_user_stop()

        switch (PreheatStatus::ConsumeResult()) {
        case PreheatStatus::Result::DoneHasFilament:
            return PhasesColdPull::cool_down;
        default:
            return PhasesColdPull::prepare_filament;
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
            FSM_CHANGE_WITH_DATA__LOGGING(PhasesColdPull::cool_down, data.fsm_data);
        };

        const auto fan_speed_stored = Temperature::fan_speed[active_extruder];
        thermalManager.set_fan_speed(active_extruder, 240);

        switch (wait_while_with_progress(PhasesColdPull::cool_down, COOLING_TIMEOUT_MILLIS, too_hot, progress)) {
        case Response::Abort:
            return PhasesColdPull::finish;
        case Response::_none:
            break;
        default:
            bsod("Invalid phase encountered.");
        }
        thermalManager.set_fan_speed(active_extruder, fan_speed_stored);
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
            FSM_CHANGE_WITH_DATA__LOGGING(PhasesColdPull::heat_up, data.fsm_data);
        };

        switch (wait_while_with_progress(PhasesColdPull::heat_up, TIMEOUT_DISABLED, too_cold, progress)) {
        case Response::Abort:
            return PhasesColdPull::finish;
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
            plan_move_by(50, 0, 0, 0, -EXTRUDE_MAXLENGTH);
            planner.synchronize();

            // mark filament unloaded
            config_store().set_filament_type(active_extruder, filament::Type::NONE);
            filament_gcodes::M70X_process_user_response(PreheatStatus::Result::DoneNoFilament, active_extruder);
        }
        return PhasesColdPull::manual_pull;
    }

    PhasesColdPull manual_pull() {
        thermalManager.disable_hotend();
        marlin_server::set_temp_to_display(0, active_extruder);

        switch (wait_for_response(PhasesColdPull::manual_pull)) {
        case Response::Continue:
            break;
        default:
            bsod("Invalid phase encountered.");
        }

        return PhasesColdPull::pull_done;
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
        case PhasesColdPull::pull_done:
            return pull_done();
        case PhasesColdPull::finish:
            return PhasesColdPull::finish;
        }
        bsod("Invalid phase encountered.");
    }

} // namespace

void M1702() {
    // Prevent filament autoload during whole ColdPull workflow.
    FS_AutoloadAutolock lock;

    PhasesColdPull phase = PhasesColdPull::introduction;
    FSM_HOLDER_WITH_DATA__LOGGING(ColdPull, PhasesColdPull::introduction, {});
    do {
        phase = get_next_phase(phase);
        FSM_CHANGE_WITH_DATA__LOGGING(phase, {});
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
