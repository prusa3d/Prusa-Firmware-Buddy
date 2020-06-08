/*
 * screen_menu_fw_update.cpp
 *
 *  Created on: Dec 18, 2019
 *      Author: Migi
 */

#include "screen_menu_fw_update.h"
#include "screens.h"
#include "sys.h"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"

/*****************************************************************************/
//MI_ALWAYS
class MI_ALWAYS : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = "Always";

public:
    MI_ALWAYS()
        : WI_SWITCH_OFF_ON_t(sys_fw_update_is_enabled() ? 0 : 1, label, 0, true, false) {}
    virtual void OnChange(size_t old_index) override {
        old_index == 0 ? sys_fw_update_disable() : sys_fw_update_enable();
        screen_dispatch_event(nullptr, WINDOW_EVENT_CLICK, (void *)index);
    }
};

/*****************************************************************************/
//MI_ON_RESTART
class MI_ON_RESTART : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = "On restart";

public:
    MI_ON_RESTART()
        : WI_SWITCH_OFF_ON_t(sys_fw_update_is_enabled() ? false : (sys_fw_update_on_restart_is_enabled() ? false : true), label, 0, sys_fw_update_is_enabled() ? false : true, false) {}
    virtual void OnChange(size_t old_index) override {
        old_index == 0 ? sys_fw_update_on_restart_disable() : sys_fw_update_on_restart_enable();
    }
};

using parent = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_Default, MI_RETURN, MI_ALWAYS, MI_ON_RESTART>;

class ScreenMenuFwUpdate : public parent {
public:
    constexpr static const char *label = "FW UPDATE";
    static void Init(screen_t *screen);
    static int CEvent(screen_t *screen, window_t *window, uint8_t event, void *param);
};

/*****************************************************************************/
//static member method definition
void ScreenMenuFwUpdate::Init(screen_t *screen) {
    Create(screen, label);
    auto *ths = reinterpret_cast<ScreenMenuFwUpdate *>(screen->pdata);
    ths->help.font = resource_font(IDR_FNT_SPECIAL);
    window_set_text(ths->help.win.id, "Select when you want\nto automatically flash\nupdated firmware\nfrom USB flash disk.");
}

int ScreenMenuFwUpdate::CEvent(screen_t *screen, window_t *window, uint8_t event, void *param) {
    ScreenMenuFwUpdate *const ths = reinterpret_cast<ScreenMenuFwUpdate *>(screen->pdata);
    if (event == WINDOW_EVENT_CLICK) {
        MI_ON_RESTART *mi_restart = &ths->Item<MI_ON_RESTART>();
        if (size_t(param) == 1) {
            mi_restart->index = sys_fw_update_on_restart_is_enabled() ? 0 : 1;
            mi_restart->Enable();
        } else {
            mi_restart->Disable();
            mi_restart->index = 0;
        }
    }

    return ths->Event(window, event, param);
}

screen_t screen_menu_fw_update = {
    0,
    0,
    ScreenMenuFwUpdate::Init,
    ScreenMenuFwUpdate::CDone,
    ScreenMenuFwUpdate::CDraw,
    ScreenMenuFwUpdate::CEvent,
    sizeof(ScreenMenuFwUpdate), //data_size
    nullptr,                    //pdata
};

extern "C" screen_t *const get_scr_menu_fw_update() { return &screen_menu_fw_update; }
