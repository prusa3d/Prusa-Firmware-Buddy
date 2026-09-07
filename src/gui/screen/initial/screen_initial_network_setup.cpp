/// @file
#include <gui/screen/initial/screen_initial_network_setup.hpp>

#include <common/marlin_client.hpp>
#include <config_store/store_instance.hpp>

bool ScreenInitialNetworkSetup::should_show() {
    return !config_store().printer_network_setup_done.get();
}

ScreenInitialNetworkSetup::ScreenInitialNetworkSetup()
    : PseudoScreenCallback {
        [] {
            marlin_client::gcode("M1703 A");
        },
    } {}
