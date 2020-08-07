/*
 * screen_menu_fw_update.cpp
 *
 *  Created on: Dec 18, 2019
 *      Author: Migi
 */

#include "sys.h"
#include "gui.hpp"
#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"

/*****************************************************************************/
//MI_ALWAYS
class MI_ALWAYS : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Always");

public:
    MI_ALWAYS()
        : WI_SWITCH_OFF_ON_t(sys_fw_update_is_enabled() ? 1 : 0, label, 0, true, false) {}
    virtual void OnChange(size_t old_index) override {
        old_index == 0 ? sys_fw_update_enable() : sys_fw_update_disable();
        Screens::Access()->ScreenEvent(nullptr, WINDOW_EVENT_CLICK, (void *)index);
    }
};

/*****************************************************************************/
//MI_ON_RESTART
class MI_ON_RESTART : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("On restart");

public:
    MI_ON_RESTART()
        : WI_SWITCH_OFF_ON_t(sys_fw_update_is_enabled() ? true : (sys_fw_update_on_restart_is_enabled() ? true : false), label, 0, sys_fw_update_is_enabled() ? false : true, false) {}
    virtual void OnChange(size_t old_index) override {
        old_index == 0 ? sys_fw_update_on_restart_enable() : sys_fw_update_on_restart_disable();
    }
};

using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_Default, MI_RETURN, MI_ALWAYS, MI_ON_RESTART>;

class ScreenMenuFwUpdate : public Screen {
public:
    constexpr static const char *const label = N_("FW UPDATE");
    ScreenMenuFwUpdate()
        : Screen(_(label)) {
        help.font = resource_font(IDR_FNT_SPECIAL);
        help.SetText(_("Select when you want\nto automatically flash\nupdated firmware\nfrom USB flash disk."));
    }
    virtual void windowEvent(window_t *sender, uint8_t ev, void *param) override;
};

void ScreenMenuFwUpdate::windowEvent(window_t *sender, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK) {
        MI_ON_RESTART *mi_restart = &Item<MI_ON_RESTART>();
        if (size_t(param) == 1) {
            mi_restart->index = sys_fw_update_on_restart_is_enabled() ? 0 : 1;
            mi_restart->Enable();
        } else {
            mi_restart->Disable();
            mi_restart->index = 0;
        }
    }
    Screen::windowEvent(sender, event, param);
}

ScreenFactory::UniquePtr GetScreenMenuFwUpdate() {
    return ScreenFactory::Screen<ScreenMenuFwUpdate>();
}
