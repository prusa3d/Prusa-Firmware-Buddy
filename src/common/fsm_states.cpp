#include <common/fsm_states.hpp>

#include <option/has_crash_detection.h>
#include <option/has_esp.h>
#include <option/has_serial_print.h>
#include <option/has_phase_stepping_calibration.h>
#include <option/has_input_shaper_calibration.h>
#include <option/has_door_sensor_calibration.h>
#include <option/has_manual_belt_tuning.h>
#include <option/has_indx.h>
#include <option/has_heaters_selftest_gcode.h>
#include <logging/log.hpp>

LOG_COMPONENT_DEF(Fsm, logging::Severity::debug);

namespace fsm {

static constexpr uint32_t score(ClientFSM fsm_type) {
    // This roughly correspons to the original FSM queues.
    // TODO In the future, we could impose a total ordering based
    //      on the underlying value of the enum.
    switch (fsm_type) {
    case ClientFSM::Wait:
        return 0;

#if HAS_SERIAL_PRINT()
    case ClientFSM::Serial_printing:
#endif
    case ClientFSM::Printing:
#if HAS_SELFTEST()
    case ClientFSM::Selftest:
    case ClientFSM::FansSelftest:
    case ClientFSM::SelftestFSensors:
#endif
#if HAS_HEATERS_SELFTEST_GCODE()
    case ClientFSM::HeatersSelftest:
#endif
#if HAS_GEARBOX_ALIGNMENT()
    case ClientFSM::GearboxAlignment:
#endif
#if HAS_DOOR_SENSOR_CALIBRATION()
    case ClientFSM::DoorSensorCalibration:
#endif
#if HAS_INDX()
    case ClientFSM::DockCalibration:
    case ClientFSM::NozzleCleanerCalibration:
#endif
#if HAS_TOOL_OFFSET_SENSOR()
    case ClientFSM::ToolOffsetsCalibration:
#endif
        return 1;

    case ClientFSM::Load_unload:
    case ClientFSM::Preheat:
#if HAS_CRASH_DETECTION()
    case ClientFSM::CrashRecovery:
#endif
    case ClientFSM::QuickPause:
    case ClientFSM::PrintPreview:
#if HAS_COLDPULL()
    case ClientFSM::ColdPull:
#endif
#if HAS_MANUAL_BELT_TUNING()
    case ClientFSM::ManualBeltTuning:
#endif
#if HAS_ESP()
    case ClientFSM::NetworkSetup:
#endif
#if HAS_PHASE_STEPPING_CALIBRATION()
    case ClientFSM::PhaseSteppingCalibration:
#endif
#if HAS_INPUT_SHAPER_CALIBRATION()
    case ClientFSM::InputShaperCalibration:
#endif
#if HAS_LOADCELL()
    case ClientFSM::NozzleCleaningFailed:
#endif
        return 2;
#if HAS_INDX()
    case ClientFSM::NozzleMismatch:
        return 3;
#endif

    case ClientFSM::SafetyTimer:
        // Show over everything except warnings
        return 4;

    case ClientFSM::Warning:
        return 5;

    case ClientFSM::_none:
        break;
    }
    abort();
}

std::optional<States::Top> States::get_top() const {
    std::optional<Top> top;

    size_t i = 0;
    for (const State &state : states) {
        const ClientFSM fsm_type = static_cast<ClientFSM>(i++);

        if (!state) {
            continue;
        }

        if (top && score(fsm_type) <= score(top->fsm_type)) {
            continue;
        }

        top = { .fsm_type = fsm_type, .data = *state };
    }

    return top;
}

void States::log() const {
#if _DEBUG
    log_debug(Fsm, "New generation %" PRIu32, state_id.to_uint32_t());
    for (size_t i = 0; i < states.size(); i++) {
        if (states[i].has_value()) {
            log_debug(Fsm, "%zu: %hhu", i, states[i]->GetPhase());
        }
    }
#endif
}

} // namespace fsm
