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

class MI_UPDATE : public WI_SWITCH_t<3> {
    constexpr static const char *const label = N_("Firmware Update");

    constexpr static const char *const str_0 = N_("Off");
    constexpr static const char *const str_1 = N_("On restart");
    constexpr static const char *const str_2 = N_("Always");

    size_t init_index() const;

public:
    MI_UPDATE();

protected:
    virtual void OnChange(size_t) override;
};

size_t MI_UPDATE::init_index() const {
    return (size_t)sys_fw_update_is_enabled()
        ? 1
        : sys_fw_update_on_restart_is_enabled()
            ? 2
            : 0;
}

MI_UPDATE::MI_UPDATE()
    : WI_SWITCH_t<3>(init_index(), _(label), 0, is_enabled_t::yes, is_hidden_t::no, _(str_0), _(str_1), _(str_2)) {
}

void MI_UPDATE::OnChange(size_t /*old_index*/) {
    if (index == 1) {
        sys_fw_update_on_restart_enable();
        sys_fw_update_disable();
    } else if (index == 2) {
        sys_fw_update_on_restart_disable();
        sys_fw_update_enable();
    } else if (index == 0) {
        sys_fw_update_on_restart_disable();
        sys_fw_update_disable();
    }
}

using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_Default, MI_RETURN, MI_UPDATE>;

class ScreenMenuFwUpdate : public Screen {
public:
    constexpr static const char *const label = N_("FW UPDATE");
    ScreenMenuFwUpdate()
        : Screen(_(label)) {
        help.font = resource_font(IDR_FNT_SPECIAL);
        help.SetText(_("Select when you want to automatically flash updated firmware from USB flash disk."));
    }
};

ScreenFactory::UniquePtr GetScreenMenuFwUpdate() {
    return ScreenFactory::Screen<ScreenMenuFwUpdate>();
}
