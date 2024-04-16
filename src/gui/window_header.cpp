#include "window_header.hpp"
#include "config.h"
#include "i18n.h"
#include "gui_media_events.hpp"
#include "time_tools.hpp"
#include "cpu_utils.hpp"
#include "img_resources.hpp"
#include "netdev.h"
#include "transfers/monitor.hpp"
#include <config_store/store_instance.hpp>
#include <guiconfig/guiconfig.h>
#include <marlin_vars.hpp>
#include "timing.h"

namespace {
constexpr uint16_t inter_item_padding { 4 };
// icon widths
constexpr Rect16::Width_t base_w { 16 };

static_assert(height(GuiDefaults::HeaderTextFont) <= GuiDefaults::HeaderItemHeight, "Text wouldn't fit into header");
static_assert(GuiDefaults::HeaderTextExtraPaddingTop <= GuiDefaults::HeaderPadding.bottom, "Text wouldn't fit into header");

constexpr Rect16::Width_t transfer_val_w { width(GuiDefaults::HeaderTextFont) * 4 };
constexpr Rect16::Width_t time_24h_w { width(GuiDefaults::HeaderTextFont) * 5 };
constexpr Rect16::Width_t time_12h_w { width(GuiDefaults::HeaderTextFont) * 8 };

constexpr Rect16::Width_t bed_text_width { width(GuiDefaults::HeaderTextFont) * 5 };

// how long the icon remains after the transfer is finished [us]
constexpr uint32_t transfer_hide_timeout { 1'000'000u };

constexpr Rect16 first_rect_doesnt_matter { 0, 0, 0, 0 }; // first rect will be replaced by first recalculation anyway

#if defined(USE_ST7789)
constexpr Rect16::Width_t label_w { 90 };
#endif // USE_ILI9488
#if defined(USE_ILI9488)
constexpr Rect16::Width_t label_w { 240 };
#endif // USE_ILI9488
} // namespace

void window_header_t::updateNetwork(uint32_t netdev_id) {
    uint32_t netdev_status = netdev_get_status(netdev_id);

    if (force_network || (netdev_id != active_netdev_id)) {
        icon_network.SetRes(window_header_t::networkIcon(netdev_id));
        active_netdev_id = netdev_id;
    }

    if (active_netdev_id == NETDEV_NODEV_ID) {
        if (icon_network.IsVisible()) {
            icon_network.Hide();
            Invalidate();
        }
    } else {
        if (force_network || (netdev_status != NETDEV_UNLINKED && !icon_network.IsVisible())) {
            icon_network.Show();
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
    auto transfer_has_issue = status ? status.value().download_has_issue : false;
    status = std::nullopt; // release internal lock

    if (transfer_progress && !last_transfer_progress) {
        transfer_hide_timer = std::nullopt;
        icon_transfer.Show();
        if (transfer_val_on) {
            transfer_val.Show();
        }
        Invalidate(); // Invalidate whole header to avoid icon leftovers in between icons
    } else if (!transfer_progress && last_transfer_progress) {
        transfer_hide_timer = ticks_us();
    }
    if (transfer_hide_timer && ticks_us() - transfer_hide_timer.value() > transfer_hide_timeout) {
        transfer_hide_timer = std::nullopt;
        icon_transfer.Hide();
        transfer_val.Hide();
        Invalidate(); // Invalidate whole header to avoid icon leftovers in between icons
    }
    if (transfer_progress && transfer_val_on && (transfer_progress != last_transfer_progress || transfer_has_issue != last_transfer_has_issue)) {
        snprintf(transfer_str, sizeof(transfer_str), "%d%%", transfer_progress.value());
        transfer_val.SetText(string_view_utf8::MakeRAM((const uint8_t *)transfer_str));
        transfer_val.SetTextColor(transfer_has_issue ? COLOR_ORANGE : COLOR_WHITE);
        transfer_val.Invalidate();
    }
    last_transfer_progress = transfer_progress;
    last_transfer_has_issue = transfer_has_issue;
}

void window_header_t::SetIcon(const img::Resource *res) {
    icon_base.SetRes(res);
    // There isn't any case where icon is removed after constructor, so we initialize label without icon and add if icon is set after constructor
    label.SetRect(Rect16(GuiDefaults::HeaderPadding.left + base_w + inter_item_padding, GuiDefaults::HeaderPadding.top + GuiDefaults::HeaderTextExtraPaddingTop, label_w, GuiDefaults::HeaderItemHeight));
    Invalidate();
}

void window_header_t::SetText(string_view_utf8 txt) {
    label.SetText(txt);
    Invalidate();
}

void window_header_t::set_show_bed_info(bool set) {
    bed_icon.set_visible(set);
    bed_text.set_visible(set);
    updateIcons(); // required because this will affect rects of items
    Invalidate();
}

void window_header_t::updateTime() {
#if !defined(USE_ST7789) // Time is not shown on ST7789
    if (time_tools::update_time()) {
        Invalidate(); // Invalidate whole header to avoid icon leftovers in between icons
    }
#endif /* !defined(USE_ST7789) */
}

void window_header_t::update_bed_info() {
    if (!bed_icon.IsVisible() || !bed_text.IsVisible()) {
        return;
    }

    static constexpr uint32_t blink_period_ms { 500 };
    uint32_t now = ticks_ms();
    if (now - bed_last_change_ms > blink_period_ms) {
        if (bed_text.GetTextColor() != COLOR_ORANGE) {
            bed_text.SetTextColor(COLOR_ORANGE);
        } else {
            bed_text.SetTextColor(COLOR_WHITE);
        }
        bed_last_change_ms = now;
    }

    snprintf(bed_str, sizeof(bed_str), "%d\xC2\xB0\x43", static_cast<int>(marlin_vars()->temp_bed.get()));
    bed_text.SetText(string_view_utf8::MakeRAM((const uint8_t *)bed_str));
    bed_text.Invalidate();
}

void window_header_t::updateAllRects() {
    // precondition: called after all other items have their status updated

    uint16_t current_offset = Width() - GuiDefaults::HeaderPadding.right;
    auto maybe_update = [&current_offset, this](auto &item, Rect16::Width_t item_width) {
        if (item.IsVisible()) {
            current_offset -= item_width;
            auto top_position { GuiDefaults::HeaderPadding.top };
            if constexpr (std::same_as<window_text_t, std::remove_cvref_t<decltype(item)>>) {
                top_position += GuiDefaults::HeaderTextExtraPaddingTop;
            }
            Rect16 new_rect { Rect16(current_offset, top_position, item_width, GuiDefaults::HeaderItemHeight) };
            if (item.GetRect() != new_rect) {

                item.SetRect(new_rect);
                Invalidate(); // invalidate everything, we've moved at least one item
            }
            current_offset -= inter_item_padding;
        }
    };

    // note: call order also means order from the right
#if !defined(USE_ST7789) // Time is not shown on ST7789
    maybe_update(time_val, time_tools::get_time_format() == time_tools::TimeFormat::_24h ? time_24h_w : time_12h_w);
#endif /* !defined(USE_ST7789) */
    maybe_update(icon_usb, icon_usb.resource()->w);
    maybe_update(icon_network, icon_network.resource()->w);
    maybe_update(icon_stealth, icon_stealth.resource()->w);
    if (transfer_val_on) {
        maybe_update(transfer_val, transfer_val_w);
    }
    maybe_update(icon_transfer, icon_transfer.resource()->w);
    maybe_update(bed_text, bed_text_width);
    maybe_update(bed_icon, bed_icon.resource()->w);

    auto label_width = current_offset - GuiDefaults::HeaderPadding.left;

    // if label needs invalidation after being attempted to set it's rect
    if (auto rc = [&]() {
            if (icon_base.IsIconValid()) {
                return label.SetRect(Rect16(GuiDefaults::HeaderPadding.left + base_w + inter_item_padding, GuiDefaults::HeaderPadding.top + GuiDefaults::HeaderTextExtraPaddingTop, label_width - base_w - inter_item_padding, GuiDefaults::HeaderItemHeight));
            } else {
                return label.SetRect(Rect16(GuiDefaults::HeaderPadding.left, GuiDefaults::HeaderPadding.top + GuiDefaults::HeaderTextExtraPaddingTop, label_width, GuiDefaults::HeaderItemHeight));
            }
        }();
        rc) {
        label.Invalidate();
    }
}

void window_header_t::updateIcons() {
    updateNetwork(netdev_get_active_id());
    updateTransfer();
    updateTime();
    update_bed_info();

    icon_stealth.set_visible(marlin_vars()->stealth_mode.get());

    updateAllRects();
}

window_header_t::window_header_t(window_t *parent, string_view_utf8 txt)
    : AddSuperWindow<window_frame_t>(parent, GuiDefaults::RectHeader)
    , icon_base(this, Rect16(GuiDefaults::HeaderPadding.left, GuiDefaults::HeaderPadding.top, base_w, GuiDefaults::HeaderItemHeight), nullptr)
    , label(this, first_rect_doesnt_matter, txt)
#if !defined(USE_ST7789) // Time is not shown on ST7789
    , time_val(this, first_rect_doesnt_matter, is_multiline::no)
#endif /* !defined(USE_ST7789) */
    , icon_usb(this, first_rect_doesnt_matter, &img::usb_20x16)
    , icon_network(this, first_rect_doesnt_matter, window_header_t::networkIcon(netdev_get_active_id()))
    , transfer_val(this, first_rect_doesnt_matter, is_multiline::no)
    , icon_transfer(this, first_rect_doesnt_matter, &img::transfer_icon_16x16)
    , icon_stealth(this, first_rect_doesnt_matter, &img::stealth_20x16)
    , bed_text(this, first_rect_doesnt_matter, is_multiline::no)
    , bed_icon(this, first_rect_doesnt_matter, &img::heatbed_16x16)
    , active_netdev_id(netdev_get_active_id())
    , active_netdev_status(netdev_get_status(active_netdev_id))
    , force_network(true)
#if defined(USE_ST7789)
    , transfer_val_on(false)
#else
    , transfer_val_on(true)
#endif

{
    label.set_font(GuiDefaults::HeaderTextFont);
    label.SetAlignment(Align_t::LeftCenter());
    transfer_val.SetAlignment(Align_t::LeftCenter());
    transfer_val.set_font(GuiDefaults::HeaderTextFont);
    icon_base.SetAlignment(Align_t::LeftCenter());
    icon_usb.SetAlignment(Align_t::LeftCenter());
    icon_network.SetAlignment(Align_t::LeftCenter());
    icon_transfer.SetAlignment(Align_t::LeftCenter());

    bed_text.set_font(GuiDefaults::HeaderTextFont);
    bed_icon.SetAlignment(Align_t::LeftCenter());

    icon_network.Hide();
    transfer_val.Hide();
    icon_transfer.Hide();

#if !defined(USE_ST7789) // Time is not shown on ST7789
    time_val.set_font(GuiDefaults::HeaderTextFont);
    time_val.SetAlignment(Align_t::RightCenter());
    time_tools::update_time();
    time_val.SetText(string_view_utf8::MakeRAM((const uint8_t *)time_tools::get_time()));
#endif /* !defined(USE_ST7789) */

    set_show_bed_info(false);
    updateMedia(GuiMediaEventsHandler::Get());
    updateIcons();

    Disable();
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
        case layout_color::blue:
            SetBlueLayout();
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
    icon_usb.set_visible(state != MediaState_t::removed);
    icon_usb.set_shadow(state != MediaState_t::inserted);
};

const img::Resource *window_header_t::networkIcon(uint32_t netdev_id) {
    const img::Resource *res = nullptr;

    switch (netdev_id) {
    case NETDEV_ETH_ID:
        res = &img::lan_16x16;
        break;
    case NETDEV_ESP_ID:
        res = &img::wifi_16x16;
        break;
    default:
        break;
    }

    return res;
}
