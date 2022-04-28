/**
 * @file screen_menu_experimental_settings_debug.cpp
 * @author Radek Vana
 * @brief experimental settings debug configuration
 * @date 2021-07-28
 */

#include "screen_menu.hpp"
#include "MItem_menus.hpp"
#include "screen_menus.hpp"
#include "eeprom.h"
#include "menu_spin_config.hpp"
#include "ScreenHandler.hpp"
#include "window_msgbox.hpp"
#include "sys.h"
#include "string.h" // memcmp
#include "MItem_experimental_tools.hpp"

/*****************************************************************************/
//Screen
using Screen = ScreenMenu<EFooter::On, MI_SAVE_AND_RETURN, MI_Z_AXIS_LEN, MI_RESET_Z_AXIS_LEN,
    MI_STEPS_PER_UNIT_X, MI_STEPS_PER_UNIT_Y, MI_STEPS_PER_UNIT_Z, MI_STEPS_PER_UNIT_E, MI_RESET_STEPS_PER_UNIT,
    MI_DIRECTION_X, MI_DIRECTION_Y, MI_DIRECTION_Z, MI_DIRECTION_E, MI_RESET_DIRECTION,
    MI_MICROSTEPS_X, MI_MICROSTEPS_Y, MI_MICROSTEPS_Z, MI_MICROSTEPS_E, MI_RESET_MICROSTEPS,
    MI_CURRENT_X, MI_CURRENT_Y, MI_CURRENT_Z, MI_CURRENT_E, MI_RESET_CURRENTS, MI_FOOTER_SETTINGS>;

class ScreenMenuExperimentalSettings : public Screen {
    static constexpr const char *const save_and_reboot = "Do you want to save changes and reboot the printer?";
    constexpr static const char *label = "Experimental Settings";
    struct values_t {
        values_t(ScreenMenuExperimentalSettings &parent)
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
            , rms_current_ma_e(parent.Item<MI_CURRENT_E>().GetVal()) {}

        int32_t z_len;
        int32_t steps_per_unit_x;
        int32_t steps_per_unit_y;
        int32_t steps_per_unit_z;
        int32_t steps_per_unit_e;
        int32_t microsteps_x;
        int32_t microsteps_y;
        int32_t microsteps_z;
        int32_t microsteps_e;
        int32_t rms_current_ma_x;
        int32_t rms_current_ma_y;
        int32_t rms_current_ma_z;
        int32_t rms_current_ma_e;

        // this is only safe as long as there are no gaps between variabes
        // all variables re 32bit now, so it is safe
        bool operator==(const values_t &other) const {
            return memcmp(this, &other, sizeof(values_t)) == 0;
        }
        bool operator!=(const values_t &other) const {
            return !(*this == other);
        }
    } initial;

    void clicked_return() {
        values_t current(*this); //ctor will handle load of values
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

public:
    ScreenMenuExperimentalSettings()
        : Screen(_(label))
        , initial(*this) {}

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t ev, void *param) override {
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
};

ScreenFactory::UniquePtr GetScreenMenuExperimentalSettings() {
    return ScreenFactory::Screen<ScreenMenuExperimentalSettings>();
}
