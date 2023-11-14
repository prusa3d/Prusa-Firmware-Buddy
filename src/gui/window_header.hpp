#pragma once

#include "gui.hpp"
#include "window_text.hpp"
#include "window_frame.hpp"
#include "media_state.hpp"

#include <time.h>

struct window_header_t : public AddSuperWindow<window_frame_t> {

    window_icon_t icon_base;
    window_roll_text_t label;
#if !defined(USE_ST7789) // Time is not shown on ST7789
    window_text_t time_val;
#endif /* !defined(USE_ST7789) */
    window_icon_t icon_usb;
    window_icon_t icon_network;
    window_text_t transfer_val;
    window_icon_t icon_transfer;
    window_text_t bed_text;
    window_icon_t bed_icon;
    uint32_t active_netdev_id;
    uint32_t active_netdev_status;
    uint32_t bed_last_change_ms { 0 }; // stores timestamp for bed blinking

    struct tm last_t;
    std::optional<bool> last_transfer_has_issue;
    std::optional<uint8_t> last_transfer_progress;
    std::optional<uint32_t> transfer_hide_timer;
    char time_str[sizeof("HH:MM AM")]; // "HH:MM AM" == Max length
    char transfer_str[sizeof("100%")];
    char bed_str[sizeof("100\xC2\xB0\x43")];
    bool force_network : 1;
    bool cpu_warning_on : 1;
    bool transfer_val_on : 1;

    void updateMedia(MediaState_t state);
    void updateNetwork(uint32_t netdev_id);
    void updateTransfer();

    void updateAllRects();
    void updateIcons();
    void updateTime();
    void update_bed_info();

    void USB_Off();
    void USB_On();
    void USB_Activate();

    static const img::Resource *networkIcon(uint32_t netdev_id);

public:
    window_header_t(window_t *parent, string_view_utf8 txt = string_view_utf8::MakeNULLSTR());

    void SetIcon(const img::Resource *res);
    void SetText(string_view_utf8 txt);

    void show_bed_info();
    void hide_bed_info();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
