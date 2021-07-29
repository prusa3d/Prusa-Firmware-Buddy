/**
 * @file screen_menu_experimental_settings.cpp
 * @author Radek Vana
 * @brief experimental settings
 * @date 2021-07-28
 */

#include "gui.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"
#include "screen_menus.hpp"
#include "status_footer.hpp"
#include "eeprom.h"
#include "menu_spin_config.hpp"
#include "ScreenHandler.hpp"
#include "window_msgbox.hpp"
#include "sys.h"

enum class ClickCommand : intptr_t { Return,
    Reset_Z };

/*****************************************************************************/
//MI_Z_AXIS_LEN
class MI_Z_AXIS_LEN : public WI_SPIN_I32_t {
    constexpr static const char *const label = N_("Z axis length");

public:
    MI_Z_AXIS_LEN()
        : WI_SPIN_I32_t(get_z_max_pos_mm(), SpinCnf::axis_z_range, _(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
};

/*****************************************************************************/
//MI_DEFAULT_Z_AXIS_LEN
class MI_DEFAULT_Z_AXIS_LEN : public WI_LABEL_t {
    static constexpr const char *const label = N_("Default Z axis length");

public:
    MI_DEFAULT_Z_AXIS_LEN()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_Z);
    }
};

/*****************************************************************************/
//MI_SAVE_AND_RETURN
class MI_SAVE_AND_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = N_("Save and return");

public:
    MI_SAVE_AND_RETURN()
        : WI_LABEL_t(_(label), IDR_PNG_folder_up_16px, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Return);
    }
};

/*****************************************************************************/
//Screen
using Screen = ScreenMenu<EFooter::On, MI_SAVE_AND_RETURN, MI_Z_AXIS_LEN, MI_DEFAULT_Z_AXIS_LEN>;

class ScreenMenuExperimentalSettings : public Screen {
    static constexpr const char *const save_and_reboot = N_("Do you want to save changes and reboot the printer?");
    constexpr static const char *label = N_("Experimental Settings");
    int32_t start_z_len;

    void return_clicked() {
        int32_t new_z_len = Item<MI_Z_AXIS_LEN>().GetVal();
        //unchanged
        if (new_z_len == start_z_len) {
            Screens::Access()->Close();
            return;
        }

        switch (MsgBoxQuestion(_(save_and_reboot), Responses_YesNoCancel)) {
        case Response::Yes:
            set_z_max_pos_mm(Item<MI_Z_AXIS_LEN>().GetVal());
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
        , start_z_len(Item<MI_Z_AXIS_LEN>().GetVal()) {}

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t ev, void *param) override {
        if (ev != GUI_event_t::CHILD_CLICK) {
            SuperWindowEvent(sender, ev, param);
            return;
        }

        switch (ClickCommand(intptr_t(param))) {
        case ClickCommand::Return:
            return_clicked();
            break;
        case ClickCommand::Reset_Z:
            Item<MI_Z_AXIS_LEN>().SetVal(default_Z_max_pos);
            menu.Invalidate(); // its broken, does not work
            break;
        }
    }
};

ScreenFactory::UniquePtr GetScreenMenuExperimentalSettings() {
    return ScreenFactory::Screen<ScreenMenuExperimentalSettings>();
}
