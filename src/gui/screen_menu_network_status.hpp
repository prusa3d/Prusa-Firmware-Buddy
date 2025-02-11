#pragma once

#include <ping_manager.hpp>
#include <gui/screen_menu.hpp>
#include <gui/MItem_network.hpp>

#include <option/buddy_enable_connect.h>
#if BUDDY_ENABLE_CONNECT()
    #include <gui/screen_menu_connect.hpp>
#endif

namespace menu_network_status {

class MI_STATS_GROUP : public IWindowMenuItem {
    static constexpr const char *label = N_("Ping | Loss");

public:
    MI_STATS_GROUP();
};

static constexpr size_t stats_text_size = 13;

class MI_STATS_GATEWAY : public WiInfo<stats_text_size> {
    static constexpr const char *label = HAS_MINI_DISPLAY() ? N_("Gateway") : N_("- Gateway");

public:
    MI_STATS_GATEWAY();
};

class MI_STATS_DNS_SERVER : public WiInfo<stats_text_size> {
    static constexpr const char *label = HAS_MINI_DISPLAY() ? N_("DNS") : N_("- DNS Server");

public:
    MI_STATS_DNS_SERVER();
};

#if BUDDY_ENABLE_CONNECT()
class MI_STATS_CONNECT : public WiInfo<stats_text_size> {
    static constexpr const char *label = HAS_MINI_DISPLAY() ? N_("Connect") : N_("- Connect");

public:
    MI_STATS_CONNECT();
};

class MI_CONNECT_IP : public WiInfo<ADDR_LEN> {
    static constexpr const char *label = N_("Connect IP");

public:
    MI_CONNECT_IP();
};
#endif

using MenuBase = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_WIFI_STATUS_t,
    MI_IP4_ADDR, MI_IP4_GWAY, MI_MAC_ADDR, MI_HOSTNAME, MI_IP4_DNS1, MI_NTP_VIA_DHCP, MI_NTP_ADDR,
#if BUDDY_ENABLE_CONNECT()
    MI_CONNECT_HOST, MI_CONNECT_IP,
#endif
    MI_STATS_GROUP,
#if BUDDY_ENABLE_CONNECT()
    MI_STATS_CONNECT,
#endif
    MI_STATS_GATEWAY, MI_STATS_DNS_SERVER //
    >;

} // namespace menu_network_status

class ScreenMenuNetworkStatus final : public menu_network_status::MenuBase {
    static constexpr const char *label = N_("NETWORK STATUS");

public:
    ScreenMenuNetworkStatus();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    enum class StatSlot {
        gateway,
        dns,
        connect,
        _cnt
    };

private:
    void update();

private:
    PingManager ping_manager_;
    bool connect_host_set_ = false;
};
