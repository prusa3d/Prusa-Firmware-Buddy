/// \file
#pragma once

#include <marlin_server_types/client_response.hpp>
#include <option/has_side_fsensor.h>
#include <option/has_extruder_fsensor.h>

enum class PhaseSelftestFSensors : PhaseUnderlyingType {
    /// Parking, toolpicking, ... - wait state
    prepare,

#if PRINTER_IS_PRUSA_MINI()
    /// The filament sensor is optional for the MINI. Here, we ask the user whether he has it or not.
    ask_mini_has_fsensor,
#endif

    /// The filament sensor devices report as not connected, ask the user to check the wiring
    not_connected,

    /// Inform the user that there should unload filament
    offer_unload,

    /// Ask the user explicitly if there is filament in the sensor
    ask_filament,

    /// Collecting samples from the sensors, wait state
    calibrating,

    /// Asks the user to insert the filament, doesn't allow to continue
    insert_filament_not_ready,

    /// Asks the user to insert the filament and allows to continue
    insert_filament_ready,

    /// Asks the user to remove the filament, doesn't allow to continue
    remove_filament_not_ready,

    /// Asks the user to insert the filament and allows to continue
    remove_filament_ready,

#if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
    /// The user is allowed to continue the selftest even if one of the sensors is not ready
    /// This is to allow him to callibrate at least some of the sensors
    /// This phase informs the user about the situation and asks him if he wants to continue in the selftest
    not_ready_confirm_continue,
#endif

    /// Selftest succeeded; finishes automatically after the user removes the filament
    success,

    failed,

    _cnt,
    _last = _cnt - 1
};

namespace ClientResponses {

inline constexpr EnumArray<PhaseSelftestFSensors, PhaseResponses, PhaseSelftestFSensors::_cnt> selftest_fsensors_responses {
    { PhaseSelftestFSensors::prepare, {} },
#if PRINTER_IS_PRUSA_MINI()
        { PhaseSelftestFSensors::ask_mini_has_fsensor, { Response::Yes, Response::No } },
#endif
        { PhaseSelftestFSensors::not_connected, { Response::Abort } },
        { PhaseSelftestFSensors::offer_unload, { Response::Continue, Response::Unload, Response::Abort } },
        { PhaseSelftestFSensors::ask_filament, { Response::Yes, Response::No, Response::Abort } },
        { PhaseSelftestFSensors::calibrating, {} },
        {
            PhaseSelftestFSensors::insert_filament_not_ready,
            {
#if HAS_SIDE_FSENSOR() // Allow continuing
                Response::Continue,
#endif
                Response::Abort,
            },
        },
        { PhaseSelftestFSensors::insert_filament_ready, { Response::Continue, Response::Abort } },
        {
            PhaseSelftestFSensors::remove_filament_not_ready,
            {
#if HAS_SIDE_FSENSOR()
                Response::Continue,
#endif
                Response::Abort,
            },
        },
        { PhaseSelftestFSensors::remove_filament_ready, { Response::Continue, Response::Abort } },
#if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
        { PhaseSelftestFSensors::not_ready_confirm_continue, { Response::Retry, Response::Continue, Response::Abort } },
#endif
        { PhaseSelftestFSensors::success, { Response::Done } },
        { PhaseSelftestFSensors::failed, { Response::Ok } },
};

} // namespace ClientResponses

constexpr inline ClientFSM client_fsm_from_phase(PhaseSelftestFSensors) { return ClientFSM::SelftestFSensors; }
