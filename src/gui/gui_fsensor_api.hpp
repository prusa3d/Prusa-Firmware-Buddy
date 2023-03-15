/**
 * @file gui_fsensor_api.hpp
 * @brief
 */

#pragma once
#include "window_msgbox.hpp"
#include "filament_sensors_handler.hpp"
#include "RAII.hpp"

namespace gui::fsensor {

// do not call directly
// msgbox calls guiloop and it would open another msgbox
// use validate_for_cyclical_calls() instead
inline void validate() {
    const filament_sensor::init_status_t status = FSensors_instance().get_active_init_status();
    switch (status) {
    case filament_sensor::init_status_t::Ok:
    case filament_sensor::init_status_t::NotReady:
    case filament_sensor::init_status_t::BothNotInitialized:
    case filament_sensor::init_status_t::SideNotInitialized:
    case filament_sensor::init_status_t::ExtruderNotInitialized:
        return;
    case filament_sensor::init_status_t::BothNotCalibrated:
    case filament_sensor::init_status_t::SideNotCalibrated:
    case filament_sensor::init_status_t::ExtruderNotCalibrated:
        MsgBoxWarning(_("The filament sensors are not fully calibrated and must be disabled to proceed.\n\nYou can calibrate them from the \"Control\" menu."), Responses_Disable);
        FSensors_instance().Disable(); // will automatically disable side sensor (or MMU) as well
        return;
    }
}

inline bool validate_for_cyclical_calls() {
    static bool can_run = true;
    if (!can_run)
        return false;
    AutoRestore ar(can_run, false);
    validate();
    return true;
}

}
