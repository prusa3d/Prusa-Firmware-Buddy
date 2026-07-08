/// @file

#include <algorithm>

#include "MItem_filament.hpp"
#include "sound.hpp"
#include "marlin_client.hpp"
#include "ScreenHandler.hpp"
#include <window_msgbox.hpp>
#include <option/has_toolchanger.h>
#include <window_dlg_wait.hpp>
#include <window_menu_callback_item.hpp>

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
    return std::ranges::any_of(VirtualToolIndex::all(), tool_has_filament);
}

void setup_item(IWindowMenuItem &item) {
    if (!PhysicalToolIndex::single_enabled_tool().has_value()) {
        // More tools are enabled, show expand icon
        item.set_show_expand_icon();
    }
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
    : IWindowMenuItem(_(label)) {
    setup_item(*this);
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
    : IWindowMenuItem(_(label)) {
    setup_item(*this);
}

void MI_UNLOAD::click(IWindowMenu &) {
#if HAS_TOOLCHANGER()
    if (!show_tool_selector_dialog({ .allow_return = true,
            .prefix_section_size = 1,
            .prefix_section_ctor = [](WindowMenuVirtual::ItemVariant &variant, int) {
                variant.emplace<WindowMenuCallbackItem>(_("Unload All"), [] {
                    // Close the select dialog
                    Screens::Access()->Close();

                    // Open the ChangeAll dialog set up for unloading everything
                    Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenChangeAllFilaments>(ScreenChangeAllFilaments::SetupUnloadAll {}));
                });
            } })) {
        return;
    }
#endif
    marlin_client::gcode("M702 W2"); // unload with return option
    sound::stop(); // TODO what is sound::stop(); doing here?
}

/*****************************************************************************/
// MI_CHANGE
MI_CHANGE::MI_CHANGE()
    : IWindowMenuItem(_(label)) {
    setup_item(*this);
}

void MI_CHANGE::Loop() {
    set_enabled(any_tool_has_filament());
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
    sound::stop(); // TODO what is sound::stop(); doing here?
}

#if HAS_TOOLCHANGER()
/*****************************************************************************/
// MI_CHANGEALL
MI_CHANGEALL::MI_CHANGEALL()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes) {}

void MI_CHANGEALL::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenChangeAllFilaments>);
}
#endif

/*****************************************************************************/
// MI_PURGE
MI_PURGE::MI_PURGE()
    : IWindowMenuItem(_(label)) {
    setup_item(*this);
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

void MI_PURGE::Loop() {
    set_enabled(any_tool_has_filament());
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
