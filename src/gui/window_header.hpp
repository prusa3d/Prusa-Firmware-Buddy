#pragma once

#include "gui.hpp"
#include "window_text.hpp"
#include "window_frame.hpp"
#include "media_state.hpp"

#include <time.h>

struct window_header_t : public AddSuperWindow<window_frame_t> {

    window_icon_t icon_base;
    window_roll_text_t label;
    window_text_t time_val;
    window_icon_t icon_usb;
    window_icon_t icon_network;
    window_text_t transfer_val;
    window_icon_t icon_transfer;
    uint32_t active_netdev_id;
    uint32_t active_netdev_status;

    struct tm last_t;
    std::optional<uint8_t> last_transfer_progress;
    std::optional<uint32_t> transfer_hide_timer;
    char time_str[sizeof("HH:MM AM")]; // "HH:MM AM" == Max length
    char transfer_str[sizeof("100%")];
    bool redraw_usb : 1;
    bool redraw_network : 1;
    bool redraw_transfer : 1;
    bool force_network : 1;
    bool cpu_warning_on : 1;
    bool transfer_val_on : 1;
    bool time_on : 1;

    void updateMedia(MediaState_t state);
    void updateNetwork(uint32_t netdev_id);
    void updateTransfer();

    void updateIcons();
    void updateTime();

    uint16_t calculate_lan_icon_x();
    uint16_t calculate_usb_icon_x();
    uint16_t calculate_transfer_val_x();
    uint16_t calculate_transfer_icon_x();

    void USB_Off();
    void USB_On();
    void USB_Activate();

    static const png::Resource *networkIcon(uint32_t netdev_id);

public:
    window_header_t(window_t *parent, string_view_utf8 txt = string_view_utf8::MakeNULLSTR());

    void SetIcon(const png::Resource *res);
    void SetText(string_view_utf8 txt);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
