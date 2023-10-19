/**
 * @file screen_menu_fw_update.cpp
 */

#include "screen_menu_fw_update.hpp"
#include "sys.h"
#include "ScreenHandler.hpp"
#include "data_exchange.hpp"

constexpr static const char *const label = N_("FW UPDATE");

#ifdef USE_ILI9488
/*****************************************************************************/
// MI_ALWAYS
MI_ALWAYS::MI_ALWAYS()
    : WI_ICON_SWITCH_OFF_ON_t(sys_fw_update_is_enabled() ? 1 : 0, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_ALWAYS::OnChange(size_t old_index) {
    old_index == 0 ? sys_fw_update_enable() : sys_fw_update_disable();
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)index);
}

/*****************************************************************************/
// MI_ON_RESTART
MI_ON_RESTART::MI_ON_RESTART()
    : WI_ICON_SWITCH_OFF_ON_t(sys_fw_update_is_enabled() ? true : data_exchange::is_fw_update_on_restart(), _(label), nullptr, sys_fw_update_is_enabled() ? is_enabled_t::no : is_enabled_t::yes, is_hidden_t::no) {}

void MI_ON_RESTART::OnChange(size_t old_index) {
    old_index == 0 ? data_exchange::fw_update_on_restart_enable() : data_exchange::fw_update_on_restart_disable();
}

ScreenMenuFwUpdate::ScreenMenuFwUpdate()
    : ScreenMenuFwUpdate__(_(label)) {
}

#else // !USE_ILI9488
static constexpr const char *en_txt_helper = N_("Select when you want to automatically flash updated firmware from USB flash disk.");
static const constexpr uint8_t blank_space_h = 10; // Visual bottom padding for HELP string

MI_UPDATE_LABEL::MI_UPDATE_LABEL()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {};

size_t MI_UPDATE::init_index() const {
    return size_t(data_exchange::is_fw_update_on_restart()
            ? 1
            : sys_fw_update_is_enabled()
            ? 2
            : 0);
}

MI_UPDATE::MI_UPDATE()
    : WI_SWITCH_t<3>(init_index(), string_view_utf8::MakeNULLSTR(), nullptr, is_enabled_t::yes, is_hidden_t::no, _(str_0), _(str_1), _(str_2)) {
}

void MI_UPDATE::OnChange(size_t /*old_index*/) {
    if (index == 1) {
        data_exchange::fw_update_on_restart_enable();
        sys_fw_update_disable();
    } else if (index == 2) {
        data_exchange::fw_update_on_restart_disable();
        sys_fw_update_enable();
    } else if (index == 0) {
        data_exchange::fw_update_on_restart_disable();
        sys_fw_update_disable();
    }
}

ScreenMenuFwUpdate::ScreenMenuFwUpdate()
    : AddSuperWindow<screen_t>(nullptr)
    , menu(this, GuiDefaults::RectScreenBody - Rect16::Height_t(get_help_h()), &container)
    , header(this)
    , help(this, Rect16(GuiDefaults::RectScreen.Left(), uint16_t(GuiDefaults::RectFooter.Top()) - get_help_h() - blank_space_h, GuiDefaults::RectScreen.Width(), get_help_h()), is_multiline::yes)
    , footer(this) {
    header.SetText(_(label));
    help.set_font(resource_font(helper_font));
    help.SetText(_(en_txt_helper));
    CaptureNormalWindow(menu); // set capture to list
}

uint16_t ScreenMenuFwUpdate::get_help_h() {
    return helper_lines * (resource_font(helper_font)->h + 1); // +1 for line paddings
}

#endif // USE_ILI9488
