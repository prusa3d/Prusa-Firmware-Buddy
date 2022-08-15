/**
 * @file gui_fsensor_api.hpp
 * @brief
 */

#pragma once
#include "window_msgbox.hpp"
#include "filament_sensor_api.hpp"
#include "RAII.hpp"

namespace gui::fsensor {

inline void validate() {
    while (true) {
        const FilamentSensors::init_status_t status = FSensors_instance().GetInitStatus();
        switch (status) {
        case FilamentSensors::init_status_t::Ok:
        case FilamentSensors::init_status_t::NotReady:
            return;
        case FilamentSensors::init_status_t::BothNotInitialized:
            if (MsgBoxWarning(_("Both MMU and printer filament sensor not initialized. Disable them?"), Responses_YesRetry) == Response::Yes) {
                FSensors_instance().Disable(); // will automatically disable MMU as well
                return;
            }
            break;
        case FilamentSensors::init_status_t::MmuNotInitialized:
            if (MsgBoxWarning(_("MMU not initialized. Disable MMU?"), Responses_YesRetry) == Response::Yes) {
                FSensors_instance().DisableMMU();
                return;
            }
            break;
        case FilamentSensors::init_status_t::PrinterNotInitialized:
#if PRINTER_TYPE == PRINTER_PRUSA_MINI
            FSensors_instance().Disable(); // will automatically disable MMU as well
            return;
#else
            if (MsgBoxWarning(_("Filament sensor not ready: calibration required. Disable printer filament sensor to continue anyway? To enable it you must perform calibration first. It is accessible from menu \"Calibrate\"."), Responses_YesRetry) == Response::Yes) {
                FSensors_instance().Disable(); // will automatically disable MMU as well
                return;
            }
#endif
            break;
        }
    }
}

inline bool validate_for_cyclical_calls() {
    static bool can_run = true;
    if (!can_run)
        return false;
    AutoRestore(can_run, false);
    validate();
    return true;
}

}
