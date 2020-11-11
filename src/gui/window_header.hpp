#pragma once

#include "gui.hpp"
#include "window_text.hpp"
#include "window_frame.hpp"

struct window_header_t : public AddSuperWindow<window_frame_t> {
    enum class header_states_t : uint8_t { OFF,
        ON,
        ACTIVE };

    window_icon_t icon_base;
    window_roll_text_t label;
    window_icon_t icon_usb;
    window_icon_t icon_lan;

    void SetIcon(int16_t id_res);
    void SetText(string_view_utf8 txt);
    header_states_t GetStateUSB() const;
    header_states_t GetStateLAN() const;
    //private:
    void USB_Off();
    void USB_On();
    void USB_Activate();
    void LAN_Off();
    void LAN_On();
    void LAN_Activate();

    void update_ETH_icon();
    window_header_t(window_t *parent, string_view_utf8 txt = string_view_utf8::MakeNULLSTR());

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
