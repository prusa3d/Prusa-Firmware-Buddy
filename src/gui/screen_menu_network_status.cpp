#include "screen_menu_network_status.hpp"

#include <netdev.h>

ScreenMenuNetworkStatus::ScreenMenuNetworkStatus()
    : ScreenMenu(_(label)) {
    Item<MI_WIFI_STATUS_t>().set_is_hidden(netdev_get_active_id() != NETDEV_ESP_ID);
}

void ScreenMenuNetworkStatus::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        update_all_updatable_items();
    }

    SuperWindowEvent(sender, event, param);
}
