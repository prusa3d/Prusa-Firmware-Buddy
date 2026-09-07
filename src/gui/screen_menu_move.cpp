#include "screen_menu_move.hpp"

#include <algorithm>

#include <marlin_client.hpp>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

#include <gui/menu_vars.h>
#include <img_resources.hpp>
#include <utils/string_builder.hpp>
#include <bsod/bsod.h>

using namespace screen_menu_move;

static const NumericInputConfig &axis_ranges_spin_config(uint8_t axis) {
    // The array has to be static so that we can have references
    static std::array<NumericInputConfig, MenuVars::AXIS_CNT> data;

    auto &c = data[axis];

    if (axis == E_AXIS) {
        c = NumericInputConfig {
            .min_value = -1000,
            .max_value = 1000,
            .unit = Unit::millimeter,
        };

    } else {
        // Ranges can change based on config_store, so udpate it each time
        const auto range = MenuVars::axis_range(axis);
        c = NumericInputConfig {
            .min_value = static_cast<float>(range.first),
            .max_value = static_cast<float>(range.second),
            .unit = Unit::millimeter,
        };
    }

    return c;
}

I_MI_AXIS::I_MI_AXIS(size_t index)
    : WiSpin(marlin_vars().native_pos[index].get(),
        axis_ranges_spin_config(index), _(MenuVars::labels[index]), nullptr, is_enabled_t::yes, is_hidden_t::no) //
{}

static const char *get_tool_state_label(ToolState state) {
    switch (state) {
    case ToolState::no_tool:
        return N_("No tool");
    case ToolState::heating:
        return N_("Heating");
    case ToolState::low_temp:
        return N_("Low temp");
    }
    bsod_unreachable();
}

DUMMY_AXIS_E::DUMMY_AXIS_E()
    : WI_FORMATABLE_LABEL_t<ToolState>(_(MenuVars::labels[MARLIN_VAR_INDEX_E]), nullptr, is_enabled_t::yes, is_hidden_t::no, ToolState::low_temp,
        [&](const std::span<char> &buffer) {
            _(get_tool_state_label(value())).copyToRAM(buffer);
        }) {
    touch_extension_only_ = true;
}

void DUMMY_AXIS_E::set_state(ToolState state) {
    UpdateValue(state); // update label
    set_enabled(state != ToolState::no_tool);
}

void DUMMY_AXIS_E::click(IWindowMenu &) {
    marlin_client::gcode("M1700 S E W2 B0"); // set filament, preheat to target, do not heat bed, return option
}

ScreenMenuMove::ScreenMenuMove()
    : ScreenMenuMove_(_("MOVE AXIS")) //
{
    ClrMenuTimeoutClose();

#if !HAS_MINI_DISPLAY()
    header.SetIcon(&img::move_16x16);
#endif

    // Either MI_AXIS_E or DUMMY_AXIS_E must be hidden for swap to work
    Item<MI_AXIS_E>().set_is_hidden(true);
    Item<MI_AXIS_E>().set_value(0);

    Item<screen_menu_move::MI_COOLDOWN>().callback = [] {
        for (auto tool : PhysicalToolIndex::all()) {
            marlin_client::set_target_nozzle(0, tool);
        }
        marlin_client::set_target_bed(0);
    };

    Item<MI_AXIS_X>().set_enabled(false);
    Item<MI_AXIS_Y>().set_enabled(false);
    Item<MI_AXIS_Z>().set_enabled(false);
    Item<MI_AXIS_E>().set_enabled(false);

    try_init();
}

void ScreenMenuMove::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        loop();
    }

    ScreenMenu::windowEvent(sender, event, param);
}

bool ScreenMenuMove::try_init() {
    if (initialized) {
        return true;
    }
    if (marlin_vars().is_processing.get() || IWindowMenuItem::edited_item() != nullptr) {
        return false;
    }
    const float nx = marlin_vars().native_pos[X_AXIS].get();
    const float ny = marlin_vars().native_pos[Y_AXIS].get();
    const float nz = marlin_vars().native_pos[Z_AXIS].get();

    Item<MI_AXIS_X>().set_value(nx);
    Item<MI_AXIS_Y>().set_value(ny);
    Item<MI_AXIS_Z>().set_value(nz);
    Item<MI_AXIS_E>().set_value(0);
    Item<MI_AXIS_X>().set_enabled(true);
    Item<MI_AXIS_Y>().set_enabled(true);
    Item<MI_AXIS_Z>().set_enabled(true);
    Item<MI_AXIS_E>().set_enabled(true);

    marlin_client::gcode("G90");
    marlin_client::gcode("M82");
    if (PhysicalToolIndex::currently_selected_opt().has_value()) {
        marlin_client::gcode("G92 E0");
    }

    e_axis_offset = 0;
    queued_pos = { { nx, ny, nz, 0 } };
    initialized = true;
    return true;
}

void ScreenMenuMove::loop() {
    if (!try_init()) {
        return;
    }

    const bool is_temp_set = (marlin_vars().active_hotend().target_nozzle > 0);

    // Update whether we can move the extruder or not
    const bool allow_e_movement = //
        marlin_vars().allow_cold_extrude // don't check cold extrusion temp if marlin doesn't
        || marlin_vars().active_hotend().temp_nozzle >= marlin_vars().extrude_min_temp;

    if (allow_e_movement == Item<MI_AXIS_E>().IsHidden()) {
        menu.menu.SwapVisibility(Item<DUMMY_AXIS_E>(), Item<MI_AXIS_E>());
    }

    // Update DUMMY_AXIS_E
    ToolState state = ToolState::no_tool;
    if (PhysicalToolIndex::currently_selected_opt().has_value()) {
        state = is_temp_set ? ToolState::heating : ToolState::low_temp;
    }
    Item<DUMMY_AXIS_E>().set_state(state);

    // Update whether MI_COOLDOWN is enabled
    Item<screen_menu_move::MI_COOLDOWN>().set_enabled(is_temp_set
#if HAS_TOOLCHANGER()
        || prusa_toolchanger.is_toolchanger_enabled() // MI_COOLDOWN is always visible on multitool
#endif
    );

    // If we finished editing E pos, reset it to zero
    if (auto &mi = Item<MI_AXIS_E>(); !mi.is_edited()) {
        e_axis_offset += mi.value();
        mi.SetVal(0);
    }

    plan_moves();
}

void ScreenMenuMove::plan_moves() {
    const xyze_float_t target_pos { {
        Item<MI_AXIS_X>().value(),
        Item<MI_AXIS_Y>().value(),
        Item<MI_AXIS_Z>().value(),
        Item<MI_AXIS_E>().value() + e_axis_offset,
    } };

    if (queued_pos == target_pos) {
        return;
    }

    ArrayStringBuilder<MARLIN_MAX_REQUEST> gcode;
    gcode.append_printf("G123 X%f Y%f Z%f",
        (double)target_pos.x,
        (double)target_pos.y,
        (double)target_pos.z);
    if (PhysicalToolIndex::currently_selected_opt().has_value()) {
        gcode.append_printf(" E%f", (double)target_pos.e);
    }
    if (marlin_client::gcode_try(gcode.str()) != marlin_client::GcodeTryResult::Submitted) {
        return;
    }

    queued_pos = target_pos;
}
