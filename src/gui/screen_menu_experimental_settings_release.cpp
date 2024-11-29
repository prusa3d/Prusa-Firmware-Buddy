/**
 * @file screen_menu_experimental_settings_release.cpp
 */

#include "screen_menu_experimental_settings.hpp"
#include "WindowMenuSpin.hpp"
#include "ScreenHandler.hpp"
#include "window_msgbox.hpp"
#include "sys.h"
#include "string.h" // memcmp
#include "MItem_experimental_tools.hpp"

void ScreenMenuExperimentalSettings::clicked_return() {
    ExperimentalSettingsValues current(*this); // ctor will handle load of values
    // unchanged
    if (current == initial) {
        Screens::Access()->Close();
        return;
    }

    switch (MsgBoxQuestion(_(save_and_reboot), Responses_YesNoCancel)) {
    case Response::Yes:
// Huge thank you to the moron who made me spend an hour of my life trying to figure why
// the value is not stored.
#if PRINTER_IS_PRUSA_COREONE() && DEVELOPMENT_ITEMS()
        Item<MI_DIRECTION_Z>().Store();
#endif
        Item<MI_Z_AXIS_LEN>().Store();
        Item<MI_STEPS_PER_UNIT_E>().Store();
        Item<MI_DIRECTION_E>().Store();

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

void ScreenMenuExperimentalSettings::windowEvent(window_t *sender, GUI_event_t ev, void *param) {
    if (ev != GUI_event_t::CHILD_CLICK) {
        ScreenMenu::windowEvent(sender, ev, param);
        return;
    }

    switch (ClickCommand(intptr_t(param))) {
    case ClickCommand::Return:
        clicked_return();
        break;
    case ClickCommand::Reset_Z:
        Item<MI_Z_AXIS_LEN>().SetVal(DEFAULT_Z_MAX_POS);
        Invalidate();
        break;
    case ClickCommand::Reset_steps:
        Item<MI_STEPS_PER_UNIT_E>().SetVal(config_store().axis_steps_per_unit_e0.default_val);
        Invalidate();
        break;
    case ClickCommand::Reset_directions:
        Item<MI_DIRECTION_E>().SetIndex(0);
        Invalidate();
        break;
    default:
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
    , steps_per_unit_e(parent.Item<MI_STEPS_PER_UNIT_E>().GetVal() * ((parent.Item<MI_DIRECTION_E>().GetIndex() == 1) ? -1 : 1))

{}
