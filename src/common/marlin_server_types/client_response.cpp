#include "client_response.hpp"
#include <option/has_crash_detection.h>
#include <option/has_serial_print.h>
#include <option/has_manual_belt_tuning.h>
#include <option/has_tool_offset_sensor.h>
#include <fsm/safety_timer_phases.hpp>
#include <fsm/print_preview_phases.hpp>

#if HAS_MANUAL_BELT_TUNING()
    #include <fsm/manual_belt_tuning_phases.hpp>
#endif

#if HAS_LOADCELL()
    #include <fsm/nozzle_cleaning_failed_phases.hpp>
#endif

#if HAS_INDX()
    #include <fsm/nozzle_mismatch_phases.hpp>
#endif

#if HAS_SELFTEST()
    #include <fsm/selftest_fsensors_phases.hpp>
#endif

#include <option/has_gearbox_alignment.h>
#if HAS_GEARBOX_ALIGNMENT()
    #include <fsm/gearbox_alignment_phases.hpp>
#endif

#include <fsm/filament_change_phases.hpp>
#include <fsm/preheat_phases.hpp>

namespace ClientResponses {

constinit const EnumArray<ClientFSM, std::span<const PhaseResponses>, ClientFSM::_count> fsm_phase_responses {
#if HAS_SERIAL_PRINT()
    { ClientFSM::Serial_printing, {} },
#endif
        { ClientFSM::Load_unload, LoadUnloadResponses },
        { ClientFSM::Preheat, ClientResponses::preheat_responses },
#if HAS_SELFTEST()
        { ClientFSM::Selftest, SelftestResponses },
        { ClientFSM::FansSelftest, FanSelftestResponses },
        { ClientFSM::SelftestFSensors, selftest_fsensors_responses },
#endif
#if HAS_HEATERS_SELFTEST_GCODE()
        { ClientFSM::HeatersSelftest, heaters_selftest_responses },
#endif
#if HAS_ESP()
        { ClientFSM::NetworkSetup, network_setup_responses },
#endif
        { ClientFSM::Printing, {} },
#if HAS_CRASH_DETECTION()
        { ClientFSM::CrashRecovery, CrashRecoveryResponses },
#endif
        { ClientFSM::QuickPause, QuickPauseResponses },
        { ClientFSM::Warning, WarningResponses },
        { ClientFSM::PrintPreview, PrintPreviewResponses },
#if HAS_COLDPULL()
        { ClientFSM::ColdPull, ColdPullResponses },
#endif
#if HAS_MANUAL_BELT_TUNING()
        { ClientFSM::ManualBeltTuning, manual_belt_tuning_responses },
#endif
#if HAS_PHASE_STEPPING_CALIBRATION()
        { ClientFSM::PhaseSteppingCalibration, phase_stepping_calibration_responses },
#endif
#if HAS_INPUT_SHAPER_CALIBRATION()
        { ClientFSM::InputShaperCalibration, input_shaper_calibration_responses },
#endif
#if HAS_GEARBOX_ALIGNMENT()
        { ClientFSM::GearboxAlignment, gearbox_alignment_responses },
#endif
#if HAS_DOOR_SENSOR_CALIBRATION()
        { ClientFSM::DoorSensorCalibration, door_sensor_calibration_responses },
#endif
#if HAS_LOADCELL()
        { ClientFSM::NozzleCleaningFailed, nozzle_cleaning_responses },
#endif
#if HAS_INDX()
        { ClientFSM::NozzleMismatch, nozzle_mismatch_responses },
        { ClientFSM::DockCalibration, dock_calibration_responses },
        { ClientFSM::NozzleCleanerCalibration, nozzle_cleaner_calibration_responses },
#endif
#if HAS_TOOL_OFFSET_SENSOR()
        { ClientFSM::ToolOffsetsCalibration, tool_offsets_calibration_responses },
#endif
        { ClientFSM::SafetyTimer, safety_timer_responses },
        { ClientFSM::Wait, {} },
};

} // namespace ClientResponses
