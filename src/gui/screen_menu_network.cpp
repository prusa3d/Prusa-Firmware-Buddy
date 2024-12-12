/**
 * @file screen_menu_network.cpp
 */
#include "screen_menu_network.hpp"
#include "printers.h"
#include "wui_api.h"
#include "netdev.h"
#include <http_lifetime.h>

ScreenMenuNetwork::ScreenMenuNetwork()
    : ScreenMenuNetwork__(_(label)) {
}
