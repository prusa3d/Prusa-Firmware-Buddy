#include "window_header.hpp"
#include "config.h"
#include "i18n.h"
#include "gui_media_events.hpp"
#include "time_tools.hpp"
#include "cpu_utils.hpp"
#include "png_resources.hpp"
#include "netdev.h"
#include "eeprom.h"
#include "transfers/monitor.hpp"

static const uint16_t span = 4;
static const uint8_t x_offset = 14;
static const uint8_t y_offset = 9;
static const uint8_t y_text_offest = 10;
// icon widths
static const Rect16::Width_t base_w(16);
static const Rect16::Width_t usb_w(36);
static const Rect16::Width_t lan_w(20);
static const Rect16::Width_t transfer_val_w(45);
static const Rect16::Width_t transfer_w(20);
static const Rect16::Width_t time_24h_w(46);
static const Rect16::Width_t time_12h_w(72);

static const Rect16::Width_t icon_usb_width(36);
static const Rect16::Width_t icon_lan_width(20);
static const Rect16::Width_t icon_transfer_width(36);
static const Rect16::Width_t icon_base_width(40);

// how long the icon remains after the transfer is finished [us]
static const uint32_t transfer_hide_timeout = 1'000'000u;

#if defined(USE_ST7789)
static const Rect16::Width_t label_w(90);
#endif // USE_ILI9488
#if defined(USE_ILI9488)
static const Rect16::Width_t label_w(240);
#endif // USE_ILI9488

void window_header_t::updateNetwork(uint32_t netdev_id) {
    uint32_t netdev_status = netdev_get_status(netdev_id);

    if (force_network || (netdev_id != active_netdev_id)) {
        icon_network.SetRes(window_header_t::networkIcon(netdev_id));
        active_netdev_id = netdev_id;
        redraw_network = true;
    }

    if (active_netdev_id == NETDEV_NODEV_ID) {
        if (icon_network.IsVisible()) {
            icon_network.Hide();
            Invalidate();
        }
    } else {
        if (force_network || (netdev_status != NETDEV_UNLINKED && !icon_network.IsVisible())) {
            icon_network.Show();
            redraw_network = true;
        }
        if (force_network || (active_netdev_status != netdev_status)) {
            if (netdev_status == NETDEV_NETIF_UP) {
                icon_network.Unshadow();
            } else if (netdev_status == NETDEV_UNLINKED) {
                icon_network.Hide();
                Invalidate();
            } else {
                icon_network.Shadow();
            }
            active_netdev_status = netdev_status;
        }
    }
    force_network = false;
}

void window_header_t::updateTransfer() {
    auto status = transfers::Monitor::instance.status();
    auto transfer_progress = status ? std::optional<uint8_t>(0.5 + status.value().progress_estimate() * 100) : std::nullopt;
    status = std::nullopt; // release internal lock

    if (transfer_progress && !last_transfer_progress) {
        transfer_hide_timer = std::nullopt;
        icon_transfer.Show();
        if (transfer_val_on) {
            transfer_val.Show();
        }
        redraw_transfer = true;
        Invalidate(); // Invalidate whole header to avoid icon leftovers in between icons
    } else if (!transfer_progress && last_transfer_progress) {
        transfer_hide_timer = ticks_us();
    }
    if (transfer_hide_timer && ticks_us() - transfer_hide_timer.value() > transfer_hide_timeout) {
        transfer_hide_timer = std::nullopt;
        icon_transfer.Hide();
        transfer_val.Hide();
        redraw_transfer = true;
        Invalidate(); // Invalidate whole header to avoid icon leftovers in between icons
    }
    if (transfer_progress && transfer_val_on && transfer_progress != last_transfer_progress) {
        snprintf(transfer_str, sizeof(transfer_str), "%d%%", transfer_progress.value());
        transfer_val.SetText(string_view_utf8::MakeRAM((const uint8_t *)transfer_str));
        transfer_val.Invalidate();
    }
    last_transfer_progress = transfer_progress;
}

void window_header_t::SetIcon(const png::Resource *res) {
    icon_base.SetRes(res);
    // There isn't any case where icon is removed after constructor, so we initialize label without icon and add if icon is set after constructor
    label.SetRect(Rect16(x_offset + base_w + span, y_text_offest, label_w, GuiDefaults::HeaderHeight - y_text_offest));
    Invalidate();
}

void window_header_t::SetText(string_view_utf8 txt) {
    label.SetText(txt);
    Invalidate();
}

uint16_t window_header_t::calculate_usb_icon_x() {
    Rect16::Width_t time_w = time_format::Get() == time_format::TF_t::TF_24H ? time_24h_w : time_12h_w;
    if (time_val.IsVisible()) {
        return Width() - x_offset - time_w - span - usb_w;
    } else {
        return Width() - x_offset - usb_w;
    }
}

uint16_t window_header_t::calculate_lan_icon_x() {
    Rect16::Width_t time_w = time_format::Get() == time_format::TF_t::TF_24H ? time_24h_w : time_12h_w;
    if (time_val.IsVisible() && icon_usb.IsVisible()) {
        return Width() - x_offset - time_w - span - usb_w - span - lan_w;
    } else if (icon_usb.IsVisible()) {
        return Width() - x_offset - usb_w - span - lan_w;
    } else if (time_val.IsVisible()) {
        return Width() - x_offset - time_w - span - lan_w;
    } else {
        return Width() - x_offset - lan_w;
    }
}

uint16_t window_header_t::calculate_transfer_val_x() {
    Rect16::Width_t time_w = time_format::Get() == time_format::TF_t::TF_24H ? time_24h_w : time_12h_w;
    uint16_t x = Width() - x_offset - transfer_val_w - span;
    x -= time_val.IsVisible() ? time_w + span : 0;
    x -= icon_usb.IsVisible() ? usb_w + span : 0;
    x -= icon_network.IsVisible() ? lan_w + span : 0;
    return x;
}

uint16_t window_header_t::calculate_transfer_icon_x() {
    Rect16::Width_t time_w = time_format::Get() == time_format::TF_t::TF_24H ? time_24h_w : time_12h_w;
    uint16_t x = Width() - x_offset - transfer_w;
    x -= time_val.IsVisible() ? time_w + span : 0;
    x -= icon_usb.IsVisible() ? usb_w + span : 0;
    x -= icon_network.IsVisible() ? lan_w + span : 0;
    x -= transfer_val.IsVisible() ? transfer_val_w + span : 0;
    return x;
}

void window_header_t::updateTime() {
    time_t t = time(nullptr);
    if (t != (time_t)-1 && time_on) { // Time is initialized in RTC (from sNTP)
        if (!time_val.IsVisible()) {
            redraw_usb = redraw_network = redraw_transfer = true; // Icons on the left of time_val have to be redrawn
            Rect16::Width_t time_w = time_format::Get() == time_format::TF_t::TF_24H ? time_24h_w : time_12h_w;
            time_val.SetRect(Rect16(Width() - x_offset - time_w, y_text_offest, time_w, GuiDefaults::HeaderHeight - y_text_offest));
            time_val.Show();
            Invalidate(); // Invalidate whole header to avoid icon leftovers in between icons
        }
        struct tm now;
        int8_t timezone_diff = variant8_get_i8(eeprom_get_var(EEVAR_TIMEZONE));
        t += timezone_diff * 3600;
        localtime_r(&t, &now);

        if (!(last_t.tm_hour == now.tm_hour && last_t.tm_min == now.tm_min) || time_format::HasChanged()) { // Time (hour || minute) has changed from previous call
            last_t = now;
            if (time_format::Get() == time_format::TF_t::TF_24H) {
                strftime(time_str, 9, "%H:%M", &now);
            } else {
                strftime(time_str, 9, "%I:%M %p", &now);
            }
            time_val.SetText(string_view_utf8::MakeRAM((const uint8_t *)time_str));
            time_val.Invalidate(); // Invalidate only time_val, when changing only time
            if (time_format::HasChanged()) {
                Rect16::Width_t time_w = time_format::Get() == time_format::TF_t::TF_24H ? time_24h_w : time_12h_w;
                time_val.SetRect(Rect16(Width() - x_offset - time_w, y_text_offest, time_w, GuiDefaults::HeaderHeight - y_text_offest));
                redraw_transfer = redraw_network = redraw_usb = true;
                Invalidate();
            }
            time_format::ClearChanged();
        }
    } else { // Time is not initialized
        if (time_val.IsVisible()) {
            time_val.Hide();
            Invalidate(); // Invalidate whole header to avoid icon leftovers in between icons
        }
    }
}

void window_header_t::updateIcons() {

    updateNetwork(netdev_get_active_id());
    updateTransfer();
    updateTime();

    // calculate usb icon's X coord
    if (redraw_usb) {
        if (icon_usb.IsVisible()) {
            icon_usb.SetRect(Rect16(calculate_usb_icon_x(), y_offset, usb_w, GuiDefaults::HeaderHeight - y_offset));
        }
        Invalidate(); // Invalidate whole header to avoid icon leftovers in between icons
    }

    // calculate lan icon's X coord
    if (redraw_network) {
        if (icon_network.IsVisible()) {
            icon_network.SetRect(Rect16(calculate_lan_icon_x(), y_offset, lan_w, GuiDefaults::HeaderHeight - y_offset));
        }
        icon_network.Invalidate();
    }

    // calculate lan icon's X coord
    if (redraw_transfer) {
        if (icon_transfer.IsVisible()) {
            icon_transfer.SetRect(Rect16(calculate_transfer_icon_x(), y_offset, transfer_w, GuiDefaults::HeaderHeight - y_offset));
        }
        if (transfer_val.IsVisible()) {
            transfer_val.SetRect(Rect16(calculate_transfer_val_x(), y_offset, transfer_val_w, GuiDefaults::HeaderHeight - y_offset));
        }
        icon_transfer.Invalidate();
        transfer_val.Invalidate();
    }
    redraw_transfer = redraw_network = redraw_usb = false;
}

window_header_t::window_header_t(window_t *parent, string_view_utf8 txt)
    : AddSuperWindow<window_frame_t>(parent, GuiDefaults::RectHeader)
    , icon_base(this, Rect16(x_offset, y_offset, base_w, GuiDefaults::HeaderHeight - y_offset), nullptr)
    , label(this, Rect16(x_offset, y_text_offest, label_w, GuiDefaults::HeaderHeight - y_text_offest), txt)
    , time_val(this, Rect16(0, y_text_offest, time_12h_w, GuiDefaults::HeaderHeight - y_text_offest), is_multiline::no)
    , icon_usb(this, Rect16(0, y_offset, usb_w, GuiDefaults::HeaderHeight - y_offset), &png::usb_32x16)
    , icon_network(this, Rect16(0, y_offset, lan_w, GuiDefaults::HeaderHeight - y_offset), window_header_t::networkIcon(netdev_get_active_id()))
    , transfer_val(this, Rect16(0, y_text_offest, transfer_val_w, GuiDefaults::HeaderHeight - y_text_offest), is_multiline::no)
    , icon_transfer(this, Rect16(0, y_offset, transfer_w, GuiDefaults::HeaderHeight - y_offset), &png::transfer_icon_16x16)
    , active_netdev_id(netdev_get_active_id())
    , active_netdev_status(netdev_get_status(active_netdev_id))
    , redraw_usb(true)
    , redraw_network(true)
    , redraw_transfer(true)
    , force_network(true)
#if PRINTER_TYPE == PRINTER_PRUSA_MINI
    , transfer_val_on(false)
    , time_on(false)
#else
    , transfer_val_on(true)
    , time_on(true)
#endif

{
#if defined(USE_ST7789)
    /// label and icon aligmnent and offset
    label.SetAlignment(Align_t::LeftBottom());
    icon_base.SetAlignment(Align_t::CenterBottom());
    icon_transfer.SetAlignment(Align_t::LeftTop());
#elif defined(USE_ILI9488)
    label.font = resource_font(IDR_FNT_SPECIAL);
    label.SetAlignment(Align_t::LeftTop());
    transfer_val.SetAlignment(Align_t::RightTop());
    icon_base.SetAlignment(Align_t::LeftTop());
    icon_usb.SetAlignment(Align_t::LeftTop());
    icon_network.SetAlignment(Align_t::LeftTop());
    icon_transfer.SetAlignment(Align_t::LeftTop());
#endif // USE_<display>

    time_val.font = resource_font(IDR_FNT_SPECIAL);
    time_val.SetAlignment(Align_t::RightTop());

    time_val.Hide();
    icon_network.Hide();
    transfer_val.Hide();
    icon_transfer.Hide();
    time_t t = time(nullptr);
    if (t != (time_t)-1) {
        int8_t timezone_diff = variant8_get_i8(eeprom_get_var(EEVAR_TIMEZONE));
        t += timezone_diff * 3600; // Add timezone difference in seconds
        struct tm now;
        localtime_r(&t, &now);
        last_t = now;
        if (time_format::Get() == time_format::TF_t::TF_24H) {
            strftime(time_str, 9, "%H:%M", &now);
        } else {
            strftime(time_str, 9, "%I:%M %p", &now);
        }
        time_val.SetText(string_view_utf8::MakeRAM((const uint8_t *)time_str));
    }
    updateMedia(GuiMediaEventsHandler::Get());
    updateIcons();

    Disable();
}

void window_header_t::USB_Off() {
    redraw_transfer = redraw_network = redraw_usb = true;
    icon_usb.Hide();
}
void window_header_t::USB_On() {
    redraw_transfer = redraw_usb = redraw_network = true;
    icon_usb.Show();
    icon_usb.Shadow();
}
void window_header_t::USB_Activate() {
    redraw_transfer = redraw_usb = redraw_network = true;
    icon_usb.Show();
    icon_usb.Unshadow();
}

void window_header_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {

    switch (event) {
    case GUI_event_t::MEDIA:
        updateMedia(MediaState_t(int(param)));
        break;
    case GUI_event_t::HEADER_COMMAND: {
        event_conversion_union uni;
        uni.pvoid = param;
        switch (uni.header.layout) {
        case layout_color::black:
            SetBlackLayout();
            break;
        case layout_color::red:
            SetRedLayout();
            break;
        case layout_color::leave_it:
            break;
        }

    }

    break;
    case GUI_event_t::LOOP:
#ifdef _DEBUG
    {
        uint16_t cpu = osGetCPUUsage();
        if (!cpu_warning_on && cpu >= 80) {
            cpu_warning_on = true;
            display::FillRect(Rect16(5, 5, 5, 5), COLOR_RED_ALERT);
        } else if (cpu_warning_on && cpu < 80) {
            cpu_warning_on = false;
            display::FillRect(Rect16(5, 5, 5, 5), GetBackColor());
        }
    }
#endif // DEBUG
    default:
        break;
    }

    updateIcons();
    SuperWindowEvent(sender, event, param);
}

void window_header_t::updateMedia(MediaState_t state) {
    switch (state) {
    case MediaState_t::inserted:
        USB_Activate();
        break;
    case MediaState_t::removed:
        USB_Off();
        break;
    case MediaState_t::error:
    default:
        USB_On();
        break;
    }
};

const png::Resource *window_header_t::networkIcon(uint32_t netdev_id) {
    const png::Resource *res = nullptr;

    switch (netdev_id) {
    case NETDEV_ETH_ID:
        res = &png::lan_16x16;
        break;
    case NETDEV_ESP_ID:
        res = &png::wifi_16x16;
        break;
    default:
        break;
    }

    return res;
}
