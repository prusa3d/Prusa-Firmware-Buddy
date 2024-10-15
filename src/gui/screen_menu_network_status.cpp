#include "screen_menu_network_status.hpp"

#include <str_utils.hpp>
#include <netdev.h>
#include <dns.h>
#include <img_resources.hpp>

#include <option/buddy_enable_connect.h>
#if BUDDY_ENABLE_CONNECT()
    #include <connect/hostname.hpp>
#endif

using namespace menu_network_status;

ScreenMenuNetworkStatus::ScreenMenuNetworkStatus()
    : MenuBase(_(label))
    , ping_manager_(ftrstd::to_underlying(StatSlot::_cnt)) {

    Item<MI_WIFI_STATUS_t>().set_is_hidden(netdev_get_active_id() != NETDEV_ESP_ID);

#if BUDDY_ENABLE_CONNECT()
    Item<MI_CONNECT_HOST>().SetLabel(_("Connect Host"));
    const bool is_connect_disabled = !config_store().connect_enabled.get();
    Item<MI_STATS_CONNECT>().set_is_hidden(is_connect_disabled);
    Item<MI_CONNECT_IP>().set_is_hidden(is_connect_disabled);
    Item<MI_CONNECT_HOST>().set_is_hidden(is_connect_disabled);
#endif

    update();
}

void ScreenMenuNetworkStatus::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        update();
    }

    ScreenMenu::windowEvent(sender, event, param);
}

void ScreenMenuNetworkStatus::update() {
    const auto device_id = netdev_get_active_id();
    const bool is_connected = netdev_get_status(device_id) == NETDEV_NETIF_UP;
    lan_t dev_config = {};
    netdev_get_ipv4_addresses(device_id, &dev_config);

    // Update pinged IP addresses
    {
        lan_t ethconfig = {};
        netdev_get_ipv4_addresses(device_id, &ethconfig);
        ping_manager_.set_ip(ftrstd::to_underlying(StatSlot::gateway), ethconfig.gw_ip4);

        // FIXME: This directly reads the global variable. It's unclear which
        // thread actually "owns" this global variable. Hopefully it's not really a
        // problem, since this is changed only in reconfigure and user would need
        // to be in a different menu to access that?
        ping_manager_.set_ip(ftrstd::to_underlying(StatSlot::dns), *dns_getserver(0));

#if BUDDY_ENABLE_CONNECT()
        // Setup Connect ping hostname - wait until we're connected, because the DNS resolution is attempted only once
        if (!connect_host_set_ && is_connected && dev_config.addr_ip4.addr) {
            std::array<char, connect_client::max_host_buf_len> hostname;
            strlcpy(hostname.data(), config_store().connect_host.get_c_str(), hostname.size());
            connect_client::decompress_host(hostname.data(), hostname.size());
            ping_manager_.set_host(ftrstd::to_underlying(StatSlot::connect), hostname.data());
            connect_host_set_ = true;
        }
#endif
    }

    EnumArray<StatSlot, PingManager::Stat, ftrstd::to_underlying(StatSlot::_cnt)> stats;
    ping_manager_.get_stats(stats.data());

#if BUDDY_ENABLE_CONNECT()
    // Update shown IP addresses
    {
        std::array<char, ADDR_LEN> str = { '-', '\0' };
        if (const auto ip = stats[StatSlot::connect].ip; ip.addr) {
            ip4addr_ntoa_r(&ip, str.data(), str.size());
        }
        Item<MI_CONNECT_IP>().ChangeInformation(str.data());
    }
#endif

    // Update stat data
    {
        const auto update_f = [&](WiInfoArray &item, StatSlot slot) {
            if (item.IsHidden()) {
                return;
            }

            auto &stat = stats[slot];

            ArrayStringBuilder<stats_text_size> sb;
            if (!stat.ip.addr && is_connected && slot == StatSlot::connect) {
                sb.append_string_view(_("DNS FAIL"));

            } else if (stat.cnt <= 0) {
                sb.append_char('-');

            } else {
                const auto rate = stat.rate();
                // rate == 0 -> no packets went through -> cannot show latency
                if (rate > 0) {
                    sb.append_printf("%i ms ", stat.latency());
                }
                sb.append_printf("%2i %%", 100 - rate);
            }
            item.ChangeInformation(sb.str());
        };
        update_f(Item<MI_STATS_GATEWAY>(), StatSlot::gateway);
#if BUDDY_ENABLE_CONNECT()
        update_f(Item<MI_STATS_CONNECT>(), StatSlot::connect);
#endif
        update_f(Item<MI_STATS_DNS_SERVER>(), StatSlot::dns);
    }
}

MI_STATS_GROUP::MI_STATS_GROUP()
    : IWindowMenuItem(_(label)) {
    set_color_scheme(&IWindowMenuItem::color_scheme_title);
}

MI_STATS_GATEWAY::MI_STATS_GATEWAY()
    : WiInfo(_(label)) {
}

MI_STATS_DNS_SERVER::MI_STATS_DNS_SERVER()
    : WiInfo(_(label)) {
}

#if BUDDY_ENABLE_CONNECT()
MI_STATS_CONNECT::MI_STATS_CONNECT()
    : WiInfo(_(label)) {
}

MI_CONNECT_IP::MI_CONNECT_IP()
    : WiInfo(_(label)) {
}
#endif
