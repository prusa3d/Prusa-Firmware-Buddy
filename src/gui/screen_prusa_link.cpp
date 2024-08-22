/**
 * @file screen_prusa_link.cpp
 */

#include "screen_prusa_link.hpp"
#include "ScreenHandler.hpp"
#include "RAII.hpp"

#include "../../lib/WUI/wui.h"

#include <array>
#include <gui/frame_qr_layout.hpp>
#include <img_resources.hpp>
#include <netdev.h>
#include "wui_api.h"
#include <config_store/store_instance.hpp>

// ----------------------------------------------------------------
// MI_PL_REGENERATE_PASSWORD
// ----------------------------------------------------------------
MI_PL_REGENERATE_PASSWORD::MI_PL_REGENERATE_PASSWORD()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_PL_REGENERATE_PASSWORD::click(IWindowMenu &) {
    std::array<char, config_store_ns::pl_password_size> password;
    wui_generate_password(password.data(), password.size());
    wui_store_password(password.data(), password.size());

    // Notify the screen so that it updates the pasword display
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, nullptr);
}

// ----------------------------------------------------------------
// MI_PL_ENABLED
// ----------------------------------------------------------------
MI_PL_ENABLED::MI_PL_ENABLED()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().prusalink_enabled.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_PL_ENABLED::OnChange([[maybe_unused]] size_t old_index) {
    config_store().prusalink_enabled.set(index);
    notify_reconfigure();
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
    : IWiInfo(string_view_utf8::MakeCPUFLASH(PRUSA_LINK_USERNAME), _(label)) {
}

MI_PL_QRCODE::MI_PL_QRCODE()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_PL_QRCODE::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenPrusaLinkQRCode>);
};

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

    case GUI_event_t::CHILD_CLICK:
        Item<MI_PL_PASSWORD_VALUE>().update_explicit();
        break;

    default:
        screen_t::windowEvent(sender, event, param);
        break;
    }
}

static constexpr const char *screen_prusa_link_qrcode = N_(
    "Link is valid only if you are connected to the same network as the printer.");

ScreenPrusaLinkQRCode::ScreenPrusaLinkQRCode()
    : screen_t(nullptr, win_type_t::normal, is_closed_on_timeout_t::no)
    , text(this, FrameQRLayout::text_rect(), is_multiline::yes, is_closed_on_click_t::no, _(screen_prusa_link_qrcode))
    , icon_phone(this, FrameQRLayout::phone_icon_rect(), &img::hand_qr_59x72)
    , qr(this, FrameQRLayout::qrcode_rect(), Align_t::Center()) {

    lan_t config = {};
    netdev_get_ipv4_addresses(netdev_get_active_id(), &config);

    qr.get_string_builder().append_printf("http://" PRUSA_LINK_USERNAME ":%s@%lu.%lu.%lu.%lu/", wui_get_password(),
        (config.addr_ip4.addr >> 0) & 0xff,
        (config.addr_ip4.addr >> 8) & 0xff,
        (config.addr_ip4.addr >> 16) & 0xff,
        (config.addr_ip4.addr >> 24) & 0xff);
}

void ScreenPrusaLinkQRCode::windowEvent(window_t *, GUI_event_t event, void *) {
    switch (event) {
    case GUI_event_t::CLICK:
    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        Screens::Access()->Close();
        break;
    default:
        break;
    }
}
