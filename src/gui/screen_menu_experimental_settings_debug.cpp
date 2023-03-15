/**
 * @file screen_menu_experimental_settings_debug.cpp
 */

#include "screen_menu_experimental_settings.hpp"
#include "eeprom.h"
#include "menu_spin_config.hpp"
#include "ScreenHandler.hpp"
#include "window_msgbox.hpp"
#include "sys.h"
#include "string.h" // memcmp
#include "MItem_experimental_tools.hpp"

void ScreenMenuExperimentalSettings::clicked_return() {
    ExperimentalSettingsValues current(*this); //ctor will handle load of values
    //unchanged
    if (current == initial) {
        Screens::Access()->Close();
        return;
    }

    switch (MsgBoxQuestion(_(save_and_reboot), Responses_YesNoCancel)) {
    case Response::Yes:
        Item<MI_Z_AXIS_LEN>().Store();

        Item<MI_STEPS_PER_UNIT_X>().Store();
        Item<MI_STEPS_PER_UNIT_Y>().Store();
        Item<MI_STEPS_PER_UNIT_Z>().Store();
        Item<MI_STEPS_PER_UNIT_E>().Store();

        Item<MI_DIRECTION_X>().Store();
        Item<MI_DIRECTION_Y>().Store();
        Item<MI_DIRECTION_Z>().Store();
        Item<MI_DIRECTION_E>().Store();

        Item<MI_MICROSTEPS_X>().Store();
        Item<MI_MICROSTEPS_Y>().Store();
        Item<MI_MICROSTEPS_Z>().Store();
        Item<MI_MICROSTEPS_E>().Store();

        Item<MI_CURRENT_X>().Store();
        Item<MI_CURRENT_Y>().Store();
        Item<MI_CURRENT_Z>().Store();
        Item<MI_CURRENT_E>().Store();

        sys_reset();
    case Response::No:
        Screens::Access()->Close();
        return;
    default:
        return; //do nothing
    }
}

ScreenMenuExperimentalSettings::ScreenMenuExperimentalSettings()
    : ScreenMenuExperimentalSettings__(_(label))
    , initial(*this) {}

void ScreenMenuExperimentalSettings::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t ev, void *param) {
    if (ev != GUI_event_t::CHILD_CLICK) {
        SuperWindowEvent(sender, ev, param);
        return;
    }

    switch (ClickCommand(intptr_t(param))) {
    case ClickCommand::Return:
        clicked_return();
        break;
    case ClickCommand::Reset_Z:
        Item<MI_Z_AXIS_LEN>().SetVal(default_Z_max_pos);
        menu.Invalidate(); // its broken, does not work
        break;
    case ClickCommand::Reset_steps:
        Item<MI_STEPS_PER_UNIT_X>().SetVal(MenuVars::GetDefaultStepsPerUnit()[0]);
        Item<MI_STEPS_PER_UNIT_Y>().SetVal(MenuVars::GetDefaultStepsPerUnit()[1]);
        Item<MI_STEPS_PER_UNIT_Z>().SetVal(MenuVars::GetDefaultStepsPerUnit()[2]);
        Item<MI_STEPS_PER_UNIT_E>().SetVal(MenuVars::GetDefaultStepsPerUnit()[3]);
        menu.Invalidate(); // its broken, does not work
        break;
    case ClickCommand::Reset_directions:
        //set index to Prusa
        Item<MI_DIRECTION_X>().SetIndex(0);
        Item<MI_DIRECTION_Y>().SetIndex(0);
        Item<MI_DIRECTION_Z>().SetIndex(0);
        Item<MI_DIRECTION_E>().SetIndex(0);
        menu.Invalidate(); // its broken, does not work
        break;
    case ClickCommand::Reset_microsteps:
        Item<MI_MICROSTEPS_X>().SetVal(MenuVars::GetDefaultMicrosteps()[0]);
        Item<MI_MICROSTEPS_Y>().SetVal(MenuVars::GetDefaultMicrosteps()[1]);
        Item<MI_MICROSTEPS_Z>().SetVal(MenuVars::GetDefaultMicrosteps()[2]);
        Item<MI_MICROSTEPS_E>().SetVal(MenuVars::GetDefaultMicrosteps()[3]);
        menu.Invalidate(); // its broken, does not work
        break;
    case ClickCommand::Reset_currents:
        Item<MI_CURRENT_X>().SetVal(MenuVars::GetDefaultCurrents()[0]);
        Item<MI_CURRENT_Y>().SetVal(MenuVars::GetDefaultCurrents()[1]);
        Item<MI_CURRENT_Z>().SetVal(MenuVars::GetDefaultCurrents()[2]);
        Item<MI_CURRENT_E>().SetVal(MenuVars::GetDefaultCurrents()[3]);
        menu.Invalidate(); // its broken, does not work
        break;
    }
}

bool ExperimentalSettingsValues::operator==(const ExperimentalSettingsValues &other) const {
    return memcmp(this, &other, sizeof(ExperimentalSettingsValues)) == 0;
}
bool ExperimentalSettingsValues::operator!=(const ExperimentalSettingsValues &other) const {
    return !(*this == other);
}

ExperimentalSettingsValues::ExperimentalSettingsValues(ScreenMenuExperimentalSettings__ &parent)
    : z_len(parent.Item<MI_Z_AXIS_LEN>().GetVal())
    , steps_per_unit_x(parent.Item<MI_STEPS_PER_UNIT_X>().GetVal() * ((parent.Item<MI_DIRECTION_X>().GetIndex() == 1) ? -1 : 1))
    , steps_per_unit_y(parent.Item<MI_STEPS_PER_UNIT_Y>().GetVal() * ((parent.Item<MI_DIRECTION_Y>().GetIndex() == 1) ? -1 : 1))
    , steps_per_unit_z(parent.Item<MI_STEPS_PER_UNIT_Z>().GetVal() * ((parent.Item<MI_DIRECTION_Z>().GetIndex() == 1) ? -1 : 1))
    , steps_per_unit_e(parent.Item<MI_STEPS_PER_UNIT_E>().GetVal() * ((parent.Item<MI_DIRECTION_E>().GetIndex() == 1) ? -1 : 1))
    , microsteps_x(parent.Item<MI_MICROSTEPS_X>().GetVal())
    , microsteps_y(parent.Item<MI_MICROSTEPS_Y>().GetVal())
    , microsteps_z(parent.Item<MI_MICROSTEPS_Z>().GetVal())
    , microsteps_e(parent.Item<MI_MICROSTEPS_E>().GetVal())
    , rms_current_ma_x(parent.Item<MI_CURRENT_X>().GetVal())
    , rms_current_ma_y(parent.Item<MI_CURRENT_Y>().GetVal())
    , rms_current_ma_z(parent.Item<MI_CURRENT_Z>().GetVal())
    , rms_current_ma_e(parent.Item<MI_CURRENT_E>().GetVal())

{}
