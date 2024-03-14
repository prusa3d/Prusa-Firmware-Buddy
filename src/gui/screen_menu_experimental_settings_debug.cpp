/**
 * @file screen_menu_experimental_settings_debug.cpp
 */

#include "screen_menu_experimental_settings.hpp"
#include "menu_spin_config.hpp"
#include "ScreenHandler.hpp"
#include "window_msgbox.hpp"
#include "sys.h"
#include "string.h" // memcmp
#include "MItem_experimental_tools.hpp"
#include <config_store/store_instance.hpp>

void ScreenMenuExperimentalSettings::clicked_return() {
    ExperimentalSettingsValues current(*this); // ctor will handle load of values
    // unchanged
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
        return; // do nothing
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
        Invalidate();
        break;
    case ClickCommand::Reset_steps:
        // Show absolute value
        Item<MI_STEPS_PER_UNIT_X>().SetVal(std::abs(config_store().axis_steps_per_unit_x.default_val));
        Item<MI_STEPS_PER_UNIT_Y>().SetVal(std::abs(config_store().axis_steps_per_unit_y.default_val));
        Item<MI_STEPS_PER_UNIT_Z>().SetVal(std::abs(config_store().axis_steps_per_unit_z.default_val));
        Item<MI_STEPS_PER_UNIT_E>().SetVal(std::abs(config_store().axis_steps_per_unit_e0.default_val));
        // Set default axis direction
        Item<MI_DIRECTION_X>().SetIndex(0);
        Item<MI_DIRECTION_Y>().SetIndex(0);
        Item<MI_DIRECTION_Z>().SetIndex(0);
        Item<MI_DIRECTION_E>().SetIndex(0);
        Invalidate();
        break;
    case ClickCommand::Reset_directions:
        // set index to Prusa
        Item<MI_DIRECTION_X>().SetIndex(0);
        Item<MI_DIRECTION_Y>().SetIndex(0);
        Item<MI_DIRECTION_Z>().SetIndex(0);
        Item<MI_DIRECTION_E>().SetIndex(0);
        Invalidate();
        break;
    case ClickCommand::Reset_microsteps:
        // 0 is valid for X and Y axis
        // it means default dependent on config_store().xy_motors_400_step
        Item<MI_MICROSTEPS_X>().SetVal(config_store().axis_microsteps_X_.default_val);
        Item<MI_MICROSTEPS_Y>().SetVal(config_store().axis_microsteps_Y_.default_val);
        Item<MI_MICROSTEPS_Z>().SetVal(config_store().axis_microsteps_Z_.default_val);
        Item<MI_MICROSTEPS_E>().SetVal(config_store().axis_microsteps_E0_.default_val);
        Invalidate();
        break;
    case ClickCommand::Reset_currents:
        // 0 is valid for X and Y axis
        // it means default dependent on config_store().xy_motors_400_step
        Item<MI_CURRENT_X>().SetVal(config_store().axis_rms_current_ma_X_.default_val);
        Item<MI_CURRENT_Y>().SetVal(config_store().axis_rms_current_ma_Y_.default_val);
        Item<MI_CURRENT_Z>().SetVal(config_store().axis_rms_current_ma_Z_.default_val);
        Item<MI_CURRENT_E>().SetVal(config_store().axis_rms_current_ma_E0_.default_val);
        Invalidate();
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
