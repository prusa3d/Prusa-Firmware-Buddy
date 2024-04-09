#pragma once

#include <gui/screen_menu.hpp>
#include <gui/MItem_network.hpp>

using ScreenMenuNetworkStatus_ = ScreenMenu<EFooter::Off, MI_RETURN, MI_WIFI_STATUS_t, MI_IP4_ADDR, MI_IP4_GWAY, MI_MAC_ADDR, MI_HOSTNAME>;

class ScreenMenuNetworkStatus final : public ScreenMenuNetworkStatus_ {
    static constexpr const char *label = N_("NETWORK STATUS");

public:
    ScreenMenuNetworkStatus();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
