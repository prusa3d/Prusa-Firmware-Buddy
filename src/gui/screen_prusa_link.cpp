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
// MI_PL_REGENERATE_PASSWORD
// ----------------------------------------------------------------
MI_PL_REGENERATE_PASSWORD::MI_PL_REGENERATE_PASSWORD()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_PL_REGENERATE_PASSWORD::click(IWindowMenu &) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)EventMask::value);
}

// ----------------------------------------------------------------
// MI_PL_ENABLED
// ----------------------------------------------------------------
MI_PL_ENABLED::MI_PL_ENABLED()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().prusalink_enabled.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_PL_ENABLED::OnChange([[maybe_unused]] size_t old_index) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)(EventMask::value | this->index));
}

MI_PL_PASSWORD_LABEL::MI_PL_PASSWORD_LABEL()
    : IWindowMenuItem(_(label), 0) {}

// ----------------------------------------------------------------
// MI_PL_PASSWORD_VALUE
// ----------------------------------------------------------------
MI_PL_PASSWORD_VALUE::MI_PL_PASSWORD_VALUE()
    : WiInfo(_(label)) {
    update_explicit();
}

void MI_PL_PASSWORD_VALUE::update_explicit() {
    ChangeInformation(config_store().prusalink_password.get().data());
}

// ----------------------------------------------------------------
// MI_PL_USER
// ----------------------------------------------------------------
MI_PL_USER::MI_PL_USER()
    : IWiInfo(string_view_utf8::MakeCPUFLASH(PRUSA_LINK_USERNAME), strlen_constexpr(PRUSA_LINK_USERNAME), _(label)) {
}

// ----------------------------------------------------------------
// ScreenMenuPrusaLink
// ----------------------------------------------------------------
ScreenMenuPrusaLink::ScreenMenuPrusaLink()
    : ScreenMenu(_("PRUSALINK")) {
    // The user might want to read the password from here, don't time it out on them.
    ClrMenuTimeoutClose();
}

void ScreenMenuPrusaLink::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CHILD_CLICK: {
        uint32_t action = ((uint32_t)param) & 0xFFFF;
        uint32_t type = ((uint32_t)param) & 0xFFFF0000;
        switch (type) {
        case MI_PL_REGENERATE_PASSWORD::EventMask::value: {
            char password[config_store_ns::pl_password_size] = { 0 };
            wui_generate_password(password, config_store_ns::pl_password_size);
            wui_store_password(password, config_store_ns::pl_password_size);
            Item<MI_PL_PASSWORD_VALUE>().update_explicit();
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
        screen_t::windowEvent(sender, event, param);
        break;
    }
}
