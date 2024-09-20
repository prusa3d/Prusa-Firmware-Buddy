#pragma once

#include "gui.hpp"
#include "window_text.hpp"
#include "window_frame.hpp"
#include "media_state.hpp"

#include <time.h>
#include <guiconfig/guiconfig.h>

#include <option/buddy_enable_connect.h>

struct window_header_t : public window_frame_t {

    window_icon_t icon_base;
    window_roll_text_t label;

#if !HAS_MINI_DISPLAY()
    // Time is not shown on ST7789
    window_text_t time_val;

    // Metrics do not fit on the mini display
    window_icon_t icon_metrics;
#endif

    window_icon_t icon_usb;
    window_icon_t icon_network;
    window_text_t transfer_val;
    window_icon_t icon_transfer;
    window_icon_t icon_stealth;
#if BUDDY_ENABLE_CONNECT()
    window_icon_t icon_connect; /// Icon switches between connect_16x16 and set_ready_16x16
#endif // BUDDY_ENABLE_CONNECT()
    window_text_t bed_text;
    window_icon_t bed_icon;
    uint32_t bed_last_change_ms { 0 }; // stores timestamp for bed blinking

    struct tm last_t;
    std::optional<bool> last_transfer_has_issue;
    std::optional<uint8_t> last_transfer_progress;
    std::optional<uint32_t> transfer_hide_timer;
    char time_str[sizeof("HH:MM AM")]; // "HH:MM AM" == Max length
    char transfer_str[sizeof("100%")];
    char bed_str[sizeof("100\xC2\xB0\x43")];
    bool cpu_warning_on : 1;
    bool transfer_val_on : 1;

    void updateMedia(MediaState_t state);
    void updateNetwork();
    void updateTransfer();

    void updateAllRects();
    void updateIcons();
    void updateTime();
    void update_bed_info();

public:
    window_header_t(window_t *parent, const string_view_utf8 &txt = string_view_utf8::MakeNULLSTR());

    void SetIcon(const img::Resource *res);
    void SetText(const string_view_utf8 &txt);

    void set_show_bed_info(bool set);

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
