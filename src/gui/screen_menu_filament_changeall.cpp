
#include "screen_menu_filament_changeall.hpp"
#include <module/prusa/toolchanger.h>
#include <filament.hpp>
#include <ScreenHandler.hpp>
#include <png_resources.hpp>
#include <marlin_client.hpp>
#include "DialogHandler.hpp"

I_MI_FilamentSelect::I_MI_FilamentSelect(const char *const label, int tool_n)
    : WI_LAMBDA_SPIN(_(label),
        (ftrstd::to_underlying(filament::Type::_last) + (filament::get_type_in_extruder(tool_n) != filament::Type::NONE ? 2 : 1)), // Allow unload only if something is loaded now
        nullptr, is_enabled_t::yes, prusa_toolchanger.is_tool_enabled(tool_n) ? is_hidden_t::no : is_hidden_t::yes,
        0, // Default "Don't change"
        [&](char *buffer) {
            const size_t index = GetIndex();
            if (index == nochange_index) {
                // Don't change
                strncpy(buffer, label_nochange, GuiDefaults::infoDefaultLen);
            } else if (index == unload_index) {
                // Unload
                strncpy(buffer, label_unload, GuiDefaults::infoDefaultLen);
            } else {
                // Print "Change to"/"Load" and filament name to buffer
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%s %s", loaded ? label_change_fil : label_load_fil, filament::get_description(filament::Type(index)).name);
            }
        })
    , loaded(filament::get_type_in_extruder(tool_n) != filament::Type::NONE) {
}

MI_FilamentApplyChanges::MI_FilamentApplyChanges()
    : WI_LABEL_t(_(label), &png::arrow_right_10x16, is_enabled_t::yes, is_hidden_t::no) {}

void MI_FilamentApplyChanges::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)this);
}

ScreenChangeAllFilaments::ScreenChangeAllFilaments()
    : ScreenChangeAllFilaments__(_(label)) {
    EnableLongHoldScreenAction();
    Screens::Access()->DisableMenuTimeout();
}

void ScreenChangeAllFilaments::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        // Get all selected filaments
        const size_t selection[] = {
            Item<MI_FilamentSelect<0>>().GetIndex(),
            Item<MI_FilamentSelect<1>>().GetIndex(),
            Item<MI_FilamentSelect<2>>().GetIndex(),
            Item<MI_FilamentSelect<3>>().GetIndex(),
            Item<MI_FilamentSelect<4>>().GetIndex(),
        };
        static_assert(tool_count == std::size(selection), "Number of tools doesn't fit");

        filament::Type new_filament[tool_count] = {}; // Filled with NONE
        filament::Type old_filament[tool_count] = {};
        bool valid[tool_count] = {}; // Nothing valid
        for (size_t tool = 0; tool < tool_count; tool++) {
            if (!prusa_toolchanger.is_tool_enabled(tool)) {
                continue; // Tool not enabled
            }

            if (selection[tool] == I_MI_FilamentSelect::nochange_index) {
                continue; // Ignore "Don't change"
            }

            if (selection[tool] == I_MI_FilamentSelect::unload_index
                && filament::get_type_in_extruder(tool) == filament::Type::NONE) {
                continue; // Ignore unload when no filament is inserted
            }

            if (selection[tool] == I_MI_FilamentSelect::unload_index) {
                new_filament[tool] = filament::Type::NONE; // Unload
            } else {
                new_filament[tool] = static_cast<filament::Type>(selection[tool]);
            }
            old_filament[tool] = filament::get_type_in_extruder(tool);
            valid[tool] = true;
        }

        // Set all temperatures
        for (size_t tool = 0; tool < tool_count; tool++) {
            if (!valid[tool]) {
                continue;
            }

            uint16_t temperature = max(filament::get_description(new_filament[tool]).nozzle, filament::get_description(old_filament[tool]).nozzle);
            marlin_set_target_nozzle(temperature, tool);
            marlin_set_display_nozzle(temperature, tool);
        }

        // Do all changes
        for (size_t tool = 0; tool < tool_count; tool++) {
            if (!valid[tool]) {
                continue;
            }

            if (new_filament[tool] == filament::Type::NONE) {
                marlin_gcode_printf("M702 T%d W2", tool); // Unload
            } else if (old_filament[tool] == filament::Type::NONE) {
                marlin_gcode_printf("M701 S\"%s\" T%d W2", filament::get_description(new_filament[tool]).name, tool); // Load
            } else {
                marlin_gcode_printf("M1600 S\"%s\" T%d R", filament::get_description(new_filament[tool]).name, tool); // Change, don't ask for unload
            }

            // Wait for one change to complete and show dialogs
            while (planner.movesplanned() || queue.has_commands_queued()) {
                gui::TickLoop();
                DialogHandler::Access().Loop();
                gui_loop();
            }
        }

        // Close this menu
        Screens::Access()->Close();
        Screens::Access()->Get()->Validate();
        return;
    }

    SuperWindowEvent(sender, event, param);
}
