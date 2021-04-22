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

class MI_UPDATE_LABEL : public WI_LABEL_t {
    static constexpr const char *const label = N_("FW Update");

public:
    MI_UPDATE_LABEL()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {};

protected:
    virtual void click(IWindowMenu &window_menu) override {};
};

class MI_UPDATE : public WI_SWITCH_t<3> {
    constexpr static const char *const str_0 = N_("Off");
    constexpr static const char *const str_1 = N_("On Restart");
    constexpr static const char *const str_2 = N_("Always");

    size_t init_index() const;

public:
    MI_UPDATE();

protected:
    virtual void OnChange(size_t) override;
};

size_t MI_UPDATE::init_index() const {
    return (size_t)sys_fw_update_on_restart_is_enabled()
        ? 1
        : sys_fw_update_is_enabled()
            ? 2
            : 0;
}

MI_UPDATE::MI_UPDATE()
    : WI_SWITCH_t<3>(init_index(), string_view_utf8::MakeNULLSTR(), 0, is_enabled_t::yes, is_hidden_t::no, _(str_0), _(str_1), _(str_2)) {
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

using MenuContainer = WinMenuContainer<MI_RETURN, MI_UPDATE_LABEL, MI_UPDATE>;

class ScreenMenuFwUpdate : public AddSuperWindow<screen_t> {
    constexpr static const char *const label = N_("FW UPDATE");
    static constexpr size_t helper_lines = 4;
    static constexpr int helper_font = IDR_FNT_SPECIAL;

    MenuContainer container;
    window_menu_t menu;
    window_header_t header;
    window_text_t help;
    status_footer_t footer;

public:
    ScreenMenuFwUpdate();

protected:
    static inline uint16_t get_help_h() {
        return helper_lines * (resource_font(helper_font)->h);
    }
};

ScreenMenuFwUpdate::ScreenMenuFwUpdate()
    : AddSuperWindow<screen_t>(nullptr)
    , menu(this, GuiDefaults::RectScreenBody - Rect16::Height_t(get_help_h()), &container)
    , header(this)
    , help(this, Rect16(GuiDefaults::RectScreen.Left(), uint16_t(GuiDefaults::RectFooter.Top()) - get_help_h(), GuiDefaults::RectScreen.Width(), get_help_h()), is_multiline::yes)
    , footer(this) {
    header.SetText(_(label));
    help.font = resource_font(helper_font);
    help.SetText(_("Select when you want to automatically flash updated firmware from USB flash disk."));
    menu.GetActiveItem()->SetFocus(); // set focus on new item//containder was not valid during construction, have to set its index again
    CaptureNormalWindow(menu);        // set capture to list
}

ScreenFactory::UniquePtr GetScreenMenuFwUpdate() {
    return ScreenFactory::Screen<ScreenMenuFwUpdate>();
}
