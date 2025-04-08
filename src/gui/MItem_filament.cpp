/**
 * @file MItem_filament.cpp
 */

#include "MItem_filament.hpp"
#include "sound.hpp"
#include "marlin_client.hpp"
#include "ScreenHandler.hpp"
#include <window_msgbox.hpp>
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
    #include "window_tool_action_box.hpp"
    #include "screen_menu_filament_changeall.hpp"
#endif
#include <config_store/store_instance.hpp>

#if HAS_TOOLCHANGER()
/// @brief  will show dialog where user can pick tool
/// @param is_tool_enabled callback function that should return true when tool is available for that action, false when not available
[[nodiscard]] ToolBox::DialogResult show_tool_selector_dialog(ToolBox::DialogToolActionBox<ToolBox::MenuPickAndGo>::IsToolEnabledFP available_for_tool = nullptr) {
    if (prusa_toolchanger.is_toolchanger_enabled()) {
        ToolBox::DialogResult result;
        {
            ToolBox::DialogToolActionBox<ToolBox::MenuPickAndGo> d;
            if (available_for_tool) {
                d.DisableNotAvailable(available_for_tool);
            }
            d.Preselect(prusa_toolchanger.get_active_tool_nr() + 1); // PickAndGo has return;
            Screens::Access()->gui_loop_until_dialog_closed();
            result = d.get_result();
        }

        // when action follows, avoid redrawing parent screen to avoid flicker back to parent screen
        // note: this has to be called after DialogToolActionBox destructor is called, otherwise it would re-validate parent screen
        if (result != ToolBox::DialogResult::Return) {
            Screens::Access()->Get()->Validate();
        }
        return result;
    }

    return ToolBox::DialogResult::Unknown;
}
#endif

/*****************************************************************************/
// MI_LOAD
MI_LOAD::MI_LOAD()
    : IWindowMenuItem(_(label)) {}

void MI_LOAD::click(IWindowMenu &) {
#if HAS_TOOLCHANGER()
    if (show_tool_selector_dialog() == ToolBox::DialogResult::Return) {
        return;
    }
#endif
    auto current_filament = config_store().get_filament_type(marlin_vars().active_extruder);
    if ((current_filament == FilamentType::none) || (MsgBoxWarning(_(warning_loaded), Responses_YesNo, 1) == Response::Yes)) {
        marlin_client::gcode("M701 W2"); // load with return option
    }
}

/*****************************************************************************/
// MI_UNLOAD
MI_UNLOAD::MI_UNLOAD()
    : IWindowMenuItem(_(label)) {}

void MI_UNLOAD::click(IWindowMenu &) {
#if HAS_TOOLCHANGER()
    if (show_tool_selector_dialog() == ToolBox::DialogResult::Return) {
        return;
    }
#endif
    marlin_client::gcode("M702 W2"); // unload with return option
    Sound_Stop(); // TODO what is Sound_Stop(); doing here?
}

/*****************************************************************************/
// MI_CHANGE
MI_CHANGE::MI_CHANGE()
    : IWindowMenuItem(_(label)) {}

bool MI_CHANGE::AvailableForTool(uint8_t tool) {
    bool has_filament_eeprom = config_store().get_filament_type(tool) != FilamentType::none;
    return has_filament_eeprom;
}

bool MI_CHANGE::AvailableForAnyTool() {
    HOTEND_LOOP() {
        if (AvailableForTool(e)) {
            return true;
        }
    }
    return false;
}

void MI_CHANGE::UpdateEnableState() {
    set_enabled(AvailableForAnyTool());
}

void MI_CHANGE::click(IWindowMenu &) {
#if HAS_TOOLCHANGER()
    if (show_tool_selector_dialog(AvailableForTool) == ToolBox::DialogResult::Return) {
        return;
    }
#endif
    marlin_client::gcode("M1600 R"); // non print filament change
    Sound_Stop(); // TODO what is Sound_Stop(); doing here?
}

#if HAS_TOOLCHANGER()
/*****************************************************************************/
// MI_CHANGEALL
MI_CHANGEALL::MI_CHANGEALL()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes) {}

void MI_CHANGEALL::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenChangeAllFilaments>);
}
#endif /*HAS_TOOLCHANGER()*/

/*****************************************************************************/
// MI_PURGE
MI_PURGE::MI_PURGE()
    : IWindowMenuItem(_(label)) {}

void MI_PURGE::click(IWindowMenu &) {
#if HAS_TOOLCHANGER()
    if (show_tool_selector_dialog(AvailableForTool) == ToolBox::DialogResult::Return) {
        return;
    }
#endif
    marlin_client::gcode("M701 L0 W2"); // load with distance 0 and return option
}

bool MI_PURGE::AvailableForTool(uint8_t tool) {
    bool has_filament_eeprom = config_store().get_filament_type(tool) != FilamentType::none;
    return has_filament_eeprom;
}

bool MI_PURGE::AvailableForAnyTool() {
    HOTEND_LOOP() {
        if (AvailableForTool(e)) {
            return true;
        }
    }
    return false;
}

void MI_PURGE::UpdateEnableState() {
    set_enabled(AvailableForAnyTool());
}

/*****************************************************************************/
// MI_AUTO_COOLDOWN
/*****************************************************************************/
bool MI_AUTO_COOLDOWN::init_index() const {
    return config_store().auto_cooldown_enabled.get();
}

void MI_AUTO_COOLDOWN::OnChange(size_t old_index) {
    if (old_index) {
        config_store().auto_cooldown_enabled.set(false);
    } else {
        config_store().auto_cooldown_enabled.set(true);
    }
}
