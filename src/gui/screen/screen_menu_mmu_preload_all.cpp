/// @file
#include "screen_menu_mmu_preload_all.hpp"

#include <WindowMenuItems.hpp>
#include <ScreenHandler.hpp>
#include <filament_list.hpp>
#include <filament_gui.hpp>
#include <config_store/store_instance.hpp>
#include <marlin_client.hpp>
#include <window_dlg_wait.hpp>
#include <multi_filament_change.hpp>

#include "gui_time.hpp"
#include "DialogHandler.hpp"

using namespace screen_mmu_preload_all;

MI_FILAMENT::MI_FILAMENT(FilamentType filament_type)
    : IWindowMenuItem()
    , filament_type(filament_type)
    , filament_name(filament_type.parameters().name) //
{
    FilamentTypeGUI::setup_menu_item(filament_type, filament_name, *this);
}

void MI_FILAMENT::click(IWindowMenu &) {
    // Set filament type for all MMU slots and issue preload commands
    for (auto slot : VirtualToolIndex::all()) {
        config_store().set_filament_type(slot, filament_type);
        marlin_client::gcode_printf("M704 P%d", slot.to_raw());

        // Wait while there are more than one command in queue to prevent race conditions
        while (marlin_vars().gqueue > 1) {
            gui::TickLoop();
            DialogHandler::Access().Loop();
            gui_loop();
        }
    }

    // Wait for all preload operations to finish
    while (marlin_vars().is_processing.get()) {
        gui::TickLoop();
        DialogHandler::Access().Loop();
        gui_loop();
    }

    Screens::Access()->Close();
}

WindowMenuMMUPreloadAll::WindowMenuMMUPreloadAll(window_t *parent, Rect16 rect)
    : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes) //
{
    generate_filament_list(filament_list, { .visible_only = true, .visible_first = true });
    setup_items();
}

int WindowMenuMMUPreloadAll::item_count() const {
    // +1 for the return item
    return 1 + filament_list.size();
}

void WindowMenuMMUPreloadAll::setup_item(ItemVariant &variant, int index) {
    if (index == 0) {
        variant.emplace<MI_RETURN>();
    } else {
        variant.emplace<MI_FILAMENT>(filament_list[index - 1]);
    }
}

ScreenMenuMMUPreloadAll::ScreenMenuMMUPreloadAll()
    : ScreenMenuBase(nullptr, _("PRELOAD ALL"), EFooter::Off) {
}
