/// @file

#include <algorithm>

#include "MItem_filament.hpp"
#include "marlin_client.hpp"
#include "ScreenHandler.hpp"
#include <window_msgbox.hpp>
#include <option/has_toolchanger.h>
#include <window_dlg_wait.hpp>

#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
    #include <gui/dialogs/dialog_tool_select.hpp>
    #include "screen_menu_filament_changeall.hpp"
#endif
#include <config_store/store_instance.hpp>

namespace {

bool tool_has_filament(VirtualToolIndex tool) {
    return config_store().get_filament_type(tool) != FilamentType::none;
}

bool any_tool_has_filament() {
    return std::ranges::any_of(VirtualToolIndex::all().skip_all_disabled(), tool_has_filament);
}

bool any_tool_configured() {
    return !PhysicalToolIndex::all().skip_all_disabled().at_end();
}

/// @brief If multiple tools have been configured, show expand icon
expands_t multi_tool_expands() {
    return (any_tool_configured() && !PhysicalToolIndex::single_enabled_tool().has_value()) ? expands_t::yes : expands_t::no;
}

} // namespace

#if HAS_TOOLCHANGER()
[[nodiscard]] bool show_tool_selector_dialog(const SelectToolDialogArgs &args) {
    const auto tool = select_tool_dialog(args);

    if (!tool) {
        return false;
    }

    marlin_client::gcode("G27 P0 Z5"); // Lift Z if not high enough
    marlin_client::gcode_printf("T%d S1 L0 D0", tool->to_raw());
    window_dlg_wait_t::wait_for_gcodes_to_finish();

    // If the pickup failed (e.g. dock error), active_extruder won't match the
    // tool we asked for. Notify the user, bail out so the caller doesn't
    // proceed with the load/unload — and skip the Validate() below so the
    // parent menu gets redrawn instead of staying stale.
    if (!stdext::holds_value(marlin_vars().active_extruder.get(), *tool)) {
        MsgBoxWarning(_("Failed to pick up the selected tool."), Responses_Ok);
        return false;
    }

    // when action follows, avoid redrawing parent screen to avoid flicker back to parent screen
    Screens::Access()->Get()->Validate();

    return true;
}
#endif

/*****************************************************************************/
// MI_LOAD
MI_LOAD::MI_LOAD()
    : IWindowMenuItem(_(label), nullptr, any_tool_configured() ? is_enabled_t::yes : is_enabled_t::no, is_hidden_t::no, multi_tool_expands()) {
}

void MI_LOAD::click(IWindowMenu &) {
#if HAS_TOOLCHANGER()
    if (!show_tool_selector_dialog({
            .allow_return = true,
        })) {
        return;
    }
#endif

    const FilamentType current_filament = match(
        marlin_vars().active_extruder.get(),
        [](VirtualToolIndex virtual_tool) -> FilamentType { return config_store().get_filament_type(virtual_tool); },
        [](NoTool) { return FilamentType::none; });
    if ((current_filament == FilamentType::none) || (MsgBoxWarning(_(warning_loaded), Responses_YesNo, 1) == Response::Yes)) {
        marlin_client::gcode("M701 W2"); // load with return option
    }
}

/*****************************************************************************/
// MI_UNLOAD
MI_UNLOAD::MI_UNLOAD()
    : IWindowMenuItem(_(label), nullptr, any_tool_configured() ? is_enabled_t::yes : is_enabled_t::no, is_hidden_t::no, multi_tool_expands()) {
}

void MI_UNLOAD::click(IWindowMenu &) {
#if HAS_TOOLCHANGER()
    if (!show_tool_selector_dialog({
            .allow_return = true,
        })) {
        return;
    }
#endif
    marlin_client::gcode("M702 W2"); // unload with return option
}

/*****************************************************************************/
// MI_CHANGE
MI_CHANGE::MI_CHANGE()
    : IWindowMenuItem(_(label), nullptr, static_cast<is_enabled_t>(any_tool_has_filament()), is_hidden_t::no, multi_tool_expands()) {
}

void MI_CHANGE::click(IWindowMenu &) {
#if HAS_TOOLCHANGER()
    if (!show_tool_selector_dialog({
            .allow_return = true,
            .tool_filter = tool_has_filament,
        })) {
        return;
    }
#endif
    marlin_client::gcode("M1600 R"); // non print filament change
}

#if HAS_TOOLCHANGER()
/*****************************************************************************/
// MI_CHANGEALL
MI_CHANGEALL::MI_CHANGEALL()
    : IWindowMenuItem(_(label), nullptr, any_tool_configured() ? is_enabled_t::yes : is_enabled_t::no, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes) {}

void MI_CHANGEALL::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenChangeAllFilaments>);
}

/*****************************************************************************/
// MI_LOAD_ALL
MI_LOAD_ALL::MI_LOAD_ALL()
    : IWindowMenuItem(_(label), nullptr, any_tool_configured() ? is_enabled_t::yes : is_enabled_t::no, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes) {}

void MI_LOAD_ALL::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenChangeAllFilaments>(ScreenChangeAllFilaments::SetupLoadAll {}));
}

/*****************************************************************************/
// MI_UNLOAD_ALL
MI_UNLOAD_ALL::MI_UNLOAD_ALL()
    : IWindowMenuItem(_(label), nullptr, static_cast<is_enabled_t>(any_tool_has_filament()), prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes) {}

void MI_UNLOAD_ALL::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenChangeAllFilaments>(ScreenChangeAllFilaments::SetupUnloadAll {}));
}
#endif

/*****************************************************************************/
// MI_PURGE
MI_PURGE::MI_PURGE()
    : IWindowMenuItem(_(label), nullptr, static_cast<is_enabled_t>(any_tool_has_filament()), is_hidden_t::no, multi_tool_expands()) {
}

void MI_PURGE::click(IWindowMenu &) {
#if HAS_TOOLCHANGER()
    if (!show_tool_selector_dialog({
            .allow_return = true,
            .tool_filter = tool_has_filament,
        })) {
        return;
    }
#endif
    marlin_client::gcode("M701 L0 W2"); // load with distance 0 and return option
}
