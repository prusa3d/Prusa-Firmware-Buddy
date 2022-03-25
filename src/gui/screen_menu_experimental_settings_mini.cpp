/**
 * @file screen_menu_experimental_settings_mini.cpp
 * @author Radek Vana
 * @brief experimental settings for MINI printer
 * @date 2021-08-03
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
using Screen = ScreenMenu<EFooter::On, MI_SAVE_AND_RETURN,
    MI_Z_AXIS_LEN, MI_RESET_Z_AXIS_LEN, MI_STEPS_PER_UNIT_E, MI_RESET_STEPS_PER_UNIT, MI_DIRECTION_E, MI_RESET_DIRECTION>;
class ScreenMenuExperimentalSettings : public Screen {
    static constexpr const char *const save_and_reboot = "Do you want to save changes and reboot the printer?";
    constexpr static const char *label = "Experimental Settings";
    struct values_t {
        values_t(ScreenMenuExperimentalSettings &parent)
            : z_len(parent.Item<MI_Z_AXIS_LEN>().GetVal())
            , steps_per_unit_e(parent.Item<MI_STEPS_PER_UNIT_E>().GetVal() * ((parent.Item<MI_DIRECTION_E>().GetIndex() == 1) ? -1 : 1)) {}

        int32_t z_len;
        int32_t steps_per_unit_e; //has stored both index and polarity

        // this is only safe as long as there are no gaps between variabes
        // all variables are 32bit now, so it is safe
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
            Item<MI_STEPS_PER_UNIT_E>().Store();
            Item<MI_DIRECTION_E>().Store();

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
            Item<MI_STEPS_PER_UNIT_E>().SetVal(MenuVars::GetDefaultStepsPerUnit()[3]);
            menu.Invalidate(); // its broken, does not work
            break;
        case ClickCommand::Reset_directions:
            Item<MI_DIRECTION_E>().SetIndex(0);
            menu.Invalidate(); // its broken, does not work
            break;
        default:
            break;
        }
    }
};

ScreenFactory::UniquePtr GetScreenMenuExperimentalSettings() {
    return ScreenFactory::Screen<ScreenMenuExperimentalSettings>();
}
