
#include "screen_menu_filament_changeall.hpp"
#include <module/prusa/toolchanger.h>
#include <filament.hpp>
#include <ScreenHandler.hpp>
#include <png_resources.hpp>
#include <marlin_client.hpp>
#include "window_dlg_wait.hpp"

IMiFilamentSelect::IMiFilamentSelect(const char *const label, int index)
    // TODO Decide on the default item and remove others
    //: WI_SWITCH_t(ftrstd::to_underlying(filament::get_type_in_extruder(index)), // Default change filament but keep the same material
    : WI_SWITCH_t((ftrstd::to_underlying(filament::Type::_last) + 1), // Default "Don't change"
        _(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_tool_enabled(index) ? is_hidden_t::no : is_hidden_t::yes,
        _(label_unload),
        _(filament::get_description(filament::Type::PLA).name), // TODO: Perhaps "Load " + "PLA", but too complicated now
        _(filament::get_description(filament::Type::PETG).name),
#if (PRINTER_TYPE == PRINTER_PRUSA_IXL)
        _(filament::get_description(filament::Type::PETG_NH).name),
#endif
        _(filament::get_description(filament::Type::ASA).name),
        _(filament::get_description(filament::Type::PC).name),
        _(filament::get_description(filament::Type::PVB).name),
        _(filament::get_description(filament::Type::ABS).name),
        _(filament::get_description(filament::Type::HIPS).name),
        _(filament::get_description(filament::Type::PP).name),
        _(filament::get_description(filament::Type::FLEX).name),
        _(filament::get_description(filament::Type::PA).name),
        _(label_nochange)) {
}

MiFilamentApplyChanges::MiFilamentApplyChanges()
    : WI_LABEL_t(_(label), &png::arrow_right_10x16, is_enabled_t::yes, is_hidden_t::no) {}

void MiFilamentApplyChanges::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)this);
}

ScreenChangeAllFilaments::ScreenChangeAllFilaments()
    : ScreenChangeAllFilaments__(_(label)) {
    EnableLongHoldScreenAction();
}

void ScreenChangeAllFilaments::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {

        const size_t selection[5] = {
            Item<MiFilamentSelect<0>>().GetIndex(),
            Item<MiFilamentSelect<1>>().GetIndex(),
            Item<MiFilamentSelect<2>>().GetIndex(),
            Item<MiFilamentSelect<3>>().GetIndex(),
            Item<MiFilamentSelect<4>>().GetIndex(),
        };
        filament::Type new_filament[5] = {}; // Filled with NONE
        filament::Type old_filament[5] = {};

        // Set all temperatures
        for (int tool = 0; tool < 5; tool++) {
            if (!prusa_toolchanger.is_tool_enabled(tool)
                || selection[tool] > ftrstd::to_underlying(filament::Type::_last) // selection > _last is "Don't change"
                || selection[tool] < ftrstd::to_underlying(filament::Type::NONE)) {
                continue;
            }

            new_filament[tool] = filament::Type(selection[tool]);
            old_filament[tool] = filament::get_type_in_extruder(tool);

            if (new_filament[tool] == filament::Type::NONE && old_filament[tool] == filament::Type::NONE) {
                continue; // Ignore change from NONE to NONE
            }

            // Set tool temperature
            uint16_t temperature = max(filament::get_description(new_filament[tool]).nozzle, filament::get_description(old_filament[tool]).nozzle);
            marlin_set_target_nozzle(temperature, tool);
            marlin_set_display_nozzle(temperature, tool);
        }

        // Do all changes
        for (int tool = 0; tool < 5; tool++) {
            if (new_filament[tool] == filament::Type::NONE && old_filament[tool] == filament::Type::NONE) {
                continue; // Filament type was set only for enabled extruders
            }

            marlin_gcode_printf("T%d S", tool); // Select tool
            if (new_filament[tool] == filament::Type::NONE) {
                marlin_gcode("M702 W2"); // Unload
            } else if (old_filament[tool] == filament::Type::NONE) {
                marlin_gcode_printf("M701 S\"%s\" W2", filament::get_description(new_filament[tool]).name); // Load
            } else {
                marlin_gcode_printf("M1600 S\"%s\" R U0", filament::get_description(new_filament[tool]).name); // Change, don't ask for unload
            }
        }

        // TODO: Some wait screen would be nice, but it breaks FSM and unload/load screen doesn't show
        // TODO: Leave the menu, but it also breaks FSM
        return;
    }

    SuperWindowEvent(sender, event, param);
}
