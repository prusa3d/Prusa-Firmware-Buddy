/**
 * @file screen_menu_hardware.cpp
 */

#include "screen_menu_hardware.hpp"
#include "screen_menu_experimental_settings.hpp"
#include "ScreenHandler.hpp"
#include "sys.h"
#include "PersistentStorage.h"
#include <config_store/store_instance.hpp>

ScreenMenuHardware::ScreenMenuHardware()
    : ScreenMenuHardware__(_(label)) {
}

void ScreenMenuHardware::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuExperimentalSettings>);
        return;
    }

    SuperWindowEvent(sender, event, param);
}

#define NOTRAN(x) string_view_utf8::MakeCPUFLASH((const uint8_t *)x)

/*****************************************************************************/
// MI_MK4_MK39
MI_MK4_MK39::MI_MK4_MK39()
    : WI_SWITCH_t<2>(!is_mk4(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no,
        NOTRAN(str_mk4), NOTRAN(str_mk39)) {
}

bool MI_MK4_MK39::is_mk4() {
    return config_store().xy_motors_400_step.get();
}

/**
 * @brief change printer type and reset the printer
 * ask user to confirm, can be aborted
 *
 * @param old_index current printer index (0: mk4, 1: mk3.5)
 */
void MI_MK4_MK39::OnChange([[maybe_unused]] size_t old_index) {
    if (MsgBoxWarning(_("Manual change of the printer type is recommended only for advanced users. To automatically select the printer type, run the Self-test."), { Response::Continue, Response::Abort, Response::_none, Response::_none }) == Response::Continue) {
        config_store().xy_motors_400_step.set(!config_store().xy_motors_400_step.get());
        PersistentStorage::erase();

        config_store().homing_sens_x.set(config_store().homing_sens_x.default_val);
        config_store().homing_sens_y.set(config_store().homing_sens_y.default_val);
        config_store().homing_bump_divisor_x.set(config_store().homing_bump_divisor_x.default_val);
        config_store().homing_bump_divisor_y.set(config_store().homing_bump_divisor_y.default_val);

        marlin_client::gcode("M914 X Y"); // Reset XY homing sensitivity
        marlin_client::gcode_printf("M906 X%u Y%u", get_rms_current_ma_x(), get_rms_current_ma_y()); // XY motor currents
        marlin_client::gcode_printf("M350 X%u Y%u", get_microsteps_x(), get_microsteps_y()); // XY motor microsteps
    } else {
        Change(0); // revert index change
    }
};
