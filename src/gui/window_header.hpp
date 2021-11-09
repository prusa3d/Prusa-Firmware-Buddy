#pragma once

#include "gui.hpp"
#include "window_text.hpp"
#include "window_frame.hpp"
#include "media_state.hpp"

struct window_header_t : public AddSuperWindow<window_frame_t> {

    window_icon_t icon_base;
    window_roll_text_t label;
    window_icon_t icon_usb;
    window_icon_t icon_network;
    uint32_t active_netdev_id;
    uint32_t active_netdev_status;

    void updateMedia(MediaState_t state);
    void updateNetwork(uint32_t netdev_id, bool force = false);

    void USB_Off();
    void USB_On();
    void USB_Activate();

    static uint32_t networkIcon(uint32_t netdev_id);

public:
    window_header_t(window_t *parent, string_view_utf8 txt = string_view_utf8::MakeNULLSTR());

    void SetIcon(int16_t id_res);
    void SetText(string_view_utf8 txt);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
