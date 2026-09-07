/// @file
#include "gcode_gearbox_alignment.hpp"

#include <bsod/bsod.h>
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include <raii/auto_restore.hpp>
#include <config_store/store_instance.hpp>
#include <M70X.hpp>
#include <mapi/motion.hpp>
#include <Marlin/src/Marlin.h>
#include <Marlin/src/module/temperature.h>
#include <Marlin/src/gcode/gcode.h>
#include <mapi/cold_extrude.hpp>
#include <utils/variant_utils.hpp>
#include <fsm/gearbox_alignment_phases.hpp>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

namespace {
class GearboxAlignmentWizard {
public:
    static constexpr PhaseGearboxAlignment first_phase = PhaseGearboxAlignment::intro;

    GearboxAlignmentWizard(PhysicalToolIndex _tool)
        : tool(_tool)
        , holder(ClientFSM::GearboxAlignment) {
    }

    void execute() {
        do {
            do_phase_and_set_next();
        } while (curr_phase != PhaseGearboxAlignment::finish);
    }

private:
    const PhysicalToolIndex tool;
    marlin_server::FSM_Holder holder;
    PhaseGearboxAlignment curr_phase = first_phase;

    void do_phase_and_set_next() {
        switch (curr_phase) {
        case PhaseGearboxAlignment::intro:
            return intro();
        case PhaseGearboxAlignment::filament_loaded_ask_unload:
            return filament_loaded_ask_unload();
        case PhaseGearboxAlignment::filament_unknown_ask_unload:
            return filament_unknown_ask_unload();
        case PhaseGearboxAlignment::loosen_screws:
            return loosen_screws();
        case PhaseGearboxAlignment::alignment:
            return alignment();
        case PhaseGearboxAlignment::tighten_screws:
            return tighten_screws();
        case PhaseGearboxAlignment::done:
            return done();
        case PhaseGearboxAlignment::finish:
            break;
        }
        bsod_unreachable();
    }

    void fsm_change(PhaseGearboxAlignment phase) {
        curr_phase = phase;
        const FSMGearboxAlignmentData data {
            .physical_tool_index = tool.to_raw(),
        };
        marlin_server::fsm_change(phase, fsm::serialize_data(data));
    }

    void finish(TestResult test_result) {
        auto sr = config_store().selftest_result.get();
        sr.set_gearbox(tool, test_result);
        config_store().selftest_result.set(sr);
        fsm_change(PhaseGearboxAlignment::finish);
        return;
    }

    void move_gear() {
        mapi::ColdExtrudeGuard cold_extrude_guard;
        constexpr const float feedrate = 8;
        mapi::extruder_schedule_turning(feedrate, -0.6f);
    }

    void filament_check() {
        if (IFSensor *sensor = GetExtruderFSensor(tool.to_raw())) {
            switch (sensor->get_state()) {
            case FilamentSensorState::HasFilament:
                fsm_change(PhaseGearboxAlignment::filament_loaded_ask_unload);
                return;
            case FilamentSensorState::NoFilament:
                fsm_change(PhaseGearboxAlignment::loosen_screws);
                return;
            case FilamentSensorState::NotInitialized:
            case FilamentSensorState::NotCalibrated:
            case FilamentSensorState::NotConnected:
            case FilamentSensorState::Disabled:
                break;
            }
        }
        fsm_change(PhaseGearboxAlignment::filament_unknown_ask_unload);
        return;
    }

    void filament_unload() {
        const auto target_tool = stdext::get_optional<VirtualToolIndex>(VirtualToolIndex::currently_selected());
        if (!target_tool.has_value()) {
            bsod("no virtual tool to unload");
        }

        filament_gcodes::M702_unload(std::nullopt, Z_AXIS_LOAD_POS, RetAndCool_t::Return, *target_tool, false);

        // check if we returned from preheat or finished the unload
        if (PreheatStatus::ConsumeResult() == PreheatStatus::Result::DoneNoFilament) {
            Temperature::setTargetHotend(0, target_tool->to_physical());
            Temperature::setTargetBed(0);
            fsm_change(PhaseGearboxAlignment::loosen_screws);
        } else {
            filament_check();
        }
    }

    void intro() {
        fsm_change(PhaseGearboxAlignment::intro);
        switch (marlin_server::wait_for_response(PhaseGearboxAlignment::intro)) {
        case Response::Skip:
            finish(TestResult::skipped);
            return;
        case Response::Continue:
#if HAS_TOOLCHANGER()
            tool_change(tool);
#endif
            filament_check();
            return;
        default:
            break;
        }
        bsod_unreachable();
    }

    void filament_loaded_ask_unload() {
        switch (marlin_server::wait_for_response(PhaseGearboxAlignment::filament_loaded_ask_unload)) {
        case Response::Abort:
            finish(TestResult::skipped);
            return;
        case Response::Unload:
            filament_unload();
            return;
        default:
            break;
        }
        bsod_unreachable();
    }

    void filament_unknown_ask_unload() {
        switch (marlin_server::wait_for_response(PhaseGearboxAlignment::filament_unknown_ask_unload)) {
        case Response::Abort:
            finish(TestResult::skipped);
            return;
        case Response::Unload:
            filament_unload();
            return;
        case Response::Continue:
            fsm_change(PhaseGearboxAlignment::loosen_screws);
            return;
        default:
            break;
        }
        bsod_unreachable();
    }

    void loosen_screws() {
        switch (marlin_server::wait_for_response(PhaseGearboxAlignment::loosen_screws)) {
        case Response::Continue:
            fsm_change(PhaseGearboxAlignment::alignment);
            return;
        case Response::Skip:
            finish(TestResult::skipped);
            return;
        default:
            break;
        }
        bsod_unreachable();
    }

    void alignment() {
        uint32_t ticks = ticks_ms();
        while (ticks_ms() - ticks < 20'000) {
            move_gear();
            idle(true);
        }
        fsm_change(PhaseGearboxAlignment::tighten_screws);
        return;
    }

    void tighten_screws() {
        for (;;) {
            switch (marlin_server::get_response_from_phase(PhaseGearboxAlignment::tighten_screws)) {
            case Response::_none:
                move_gear();
                idle(true);
                continue;
            case Response::Continue:
                fsm_change(PhaseGearboxAlignment::done);
                return;
            default:
                continue;
            }
        }
    }

    void done() {
        switch (marlin_server::wait_for_response(PhaseGearboxAlignment::done)) {
        case Response::Continue:
            finish(TestResult::passed);
            return;
        default:
            break;
        }
        bsod_unreachable();
    }
};
}; // namespace
/** \addtogroup G-Codes
 * @{
 */

/**
 *### M1979 [ T ]: Gearbox alignment dialog (internal GCode)
 *
 *  - `T` - Set the tool
 *           1. if tool is not specified, active toolhead is picked
 *           2. if tool is out of range, gcode will do nothing
 *
 *#### Usage
 *
 *    M1979 T2 ; Run the gearbox alignment wizard on toolhead 2
 *    M1979    ; Run gearbox alignment wizard on active toolhead
 *
 */
void PrusaGcodeSuite::M1979() {
    const std::optional<PhysicalToolIndex> tool = stdext::get_optional<PhysicalToolIndex>(GcodeSuite::get_target_physical_from_command());

    // Wrong value specified in T argument
    if (!tool.has_value()) {
        return;
    }
    GearboxAlignmentWizard ga(*tool);
    ga.execute();
}

/** @}*/
