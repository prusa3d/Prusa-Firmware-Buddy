/**
 * @file screen_prusa_link.cpp
 */

#include "screen_prusa_link.hpp"
#include "ScreenHandler.hpp"
#include "RAII.hpp"

#include "../../lib/WUI/wui.h"

#include <array>

#include "wui_api.h"
#include <config_store/store_instance.hpp>

// ----------------------------------------------------------------
// GUI Prusa Link Password regenerate
MI_PL_REGENERATE_PASSWORD::MI_PL_REGENERATE_PASSWORD()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_PL_REGENERATE_PASSWORD::click(IWindowMenu &) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)EventMask::value);
}

// ----------------------------------------------------------------
// GUI Prusa Link start after printer startup
MI_PL_ENABLED::MI_PL_ENABLED()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().prusalink_enabled.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_PL_ENABLED::OnChange([[maybe_unused]] size_t old_index) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)(EventMask::value | this->index));
}

MI_PL_PASSWORD_LABEL::MI_PL_PASSWORD_LABEL()
    : WI_LABEL_t(_(label), 0) {}

void MI_PL_PASSWORD_VALUE::printExtension(Rect16 extension_rect, [[maybe_unused]] color_t color_text, color_t color_back, [[maybe_unused]] ropfn raster_op) const {
    render_text_align(extension_rect, string_view_utf8::MakeRAM(reinterpret_cast<const uint8_t *>(passwd_buffer)), GuiDefaults::FontMenuSpecial, color_back, (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingItems, Align_t::RightCenter());
}

void MI_PL_PASSWORD_VALUE::print_password(const char *passwd) {
    snprintf(passwd_buffer, PASSWD_STR_LENGTH + 1, "%s", passwd);
    InValidateExtension();
}

MI_PL_PASSWORD_VALUE::MI_PL_PASSWORD_VALUE()
    : WI_LABEL_t(_(label), PASSWD_STR_LENGTH * GuiDefaults::FontMenuSpecial->w) {}

void MI_PL_USER::printExtension(Rect16 extension_rect, [[maybe_unused]] color_t color_text, color_t color_back, [[maybe_unused]] ropfn raster_op) const {
    render_text_align(extension_rect, string_view_utf8::MakeRAM(reinterpret_cast<const uint8_t *>(PRUSA_LINK_USERNAME)), GuiDefaults::FontMenuSpecial, color_back, (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingItems, Align_t::RightCenter());
}

MI_PL_USER::MI_PL_USER()
    : WI_LABEL_t(_(label), (sizeof(PRUSA_LINK_USERNAME) + 1) * GuiDefaults::FontMenuSpecial->w) {}

ScreenMenuPrusaLink::ScreenMenuPrusaLink()
    : AddSuperWindow<screen_t>(nullptr, win_type_t::normal, is_closed_on_timeout_t::no)
    , menu(this, GuiDefaults::RectScreenBody - Rect16::Height_t(canvas_font_height()), &container)
    , header(this) {
    header.SetText(_("PRUSALINK"));
    CaptureNormalWindow(menu); // set capture to list
    display_passwd(wui_get_password());
    // The user might want to read the password from here, don't time it out on them.
    ClrMenuTimeoutClose();
}

void ScreenMenuPrusaLink::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CHILD_CLICK: {
        uint32_t action = ((uint32_t)param) & 0xFFFF;
        uint32_t type = ((uint32_t)param) & 0xFFFF0000;
        switch (type) {
        case MI_PL_REGENERATE_PASSWORD::EventMask::value: {
            char password[config_store_ns::pl_password_size] = { 0 };
            wui_generate_password(password, config_store_ns::pl_password_size);
            wui_store_password(password, config_store_ns::pl_password_size);
            display_passwd(password);
            break;
        }
        case MI_PL_ENABLED::EventMask::value:
            config_store().prusalink_enabled.set(static_cast<uint8_t>(action));
            notify_reconfigure();
            break;
        default:
            break;
        }
    } break;
    default:
        SuperWindowEvent(sender, event, param);
        break;
    }
}
