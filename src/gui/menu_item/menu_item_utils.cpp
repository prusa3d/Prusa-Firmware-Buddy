/// @file
#include "menu_item_utils.hpp"

#include <marlin_vars.hpp>
#include <img_resources.hpp>

void loop_gcode_inject_menu_item(IWindowMenuItem &item, LoopGCodeInjectMenuItemArgs args) {
    const bool inject_queue_empty = marlin_vars().inject_queue_empty;

    if (args.update_enabled) {
        item.set_enabled(inject_queue_empty && args.enabled);
    }

    if (args.update_icon) {
        item.SetIconId(inject_queue_empty ? nullptr : img::spinner_16x16_animated());
    }
}
