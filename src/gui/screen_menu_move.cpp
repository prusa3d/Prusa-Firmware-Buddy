#include "screen_menu_move.hpp"

#include <algorithm>

#include <marlin_client.hpp>
#include <option/has_toolchanger.h>
#include <gui/menu_vars.h>
#include <img_resources.hpp>

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

void DUMMY_AXIS_E::click(IWindowMenu &) {
    marlin_client::gcode("M1700 S E W2 B0"); // set filament, preheat to target, do not heat bed, return option
}

DUMMY_AXIS_E::DUMMY_AXIS_E()
    : WI_FORMATABLE_LABEL_t<int>(_(MenuVars::labels[MARLIN_VAR_INDEX_E]), nullptr, is_enabled_t::yes, is_hidden_t::no, 0,
        // this lambda is used during print, but does require item to be invalidated
        [&](const std::span<char> &buffer) {
            const char *label_str = value() ? N_("Heating") : N_("Low temp");
            _(label_str).copyToRAM(buffer);
        }) {
    touch_extension_only_ = true;
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

    Item<screen_menu_move::MI_COOLDOWN>().callback = [] {
        HOTEND_LOOP() {
            marlin_client::set_target_nozzle(0, e);
        }
        marlin_client::set_display_nozzle(0);
        marlin_client::set_target_bed(0);
    };

    marlin_client::gcode("G90"); // Set to Absolute Positioning
    marlin_client::gcode("M82"); // Set extruder to absolute mode
    marlin_client::gcode("G92 E0"); // Reset position before change
}

void ScreenMenuMove::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        loop();
    }

    ScreenMenu::windowEvent(sender, event, param);
}

void ScreenMenuMove::loop() {
    const bool is_temp_set = (marlin_vars().active_hotend().target_nozzle > 0);

    // Update whether we can move the extruder or not
    const bool allow_e_movement = //
        marlin_vars().allow_cold_extrude // don't check cold extrusion temp if marlin doesn't
        || marlin_vars().active_hotend().temp_nozzle >= marlin_vars().extrude_min_temp;

    if (allow_e_movement == Item<MI_AXIS_E>().IsHidden()) {
        menu.menu.SwapVisibility(Item<DUMMY_AXIS_E>(), Item<MI_AXIS_E>());
    }

    // Update DUMMY_AXIS_E label (too cold vs heating up)
    Item<DUMMY_AXIS_E>().UpdateValue(is_temp_set);

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
    gcode.append_printf("G123 X%f Y%f Z%f E%f",
        (double)target_pos.x,
        (double)target_pos.y,
        (double)target_pos.z,
        (double)target_pos.e //
    );
    if (marlin_client::gcode_try(gcode.str()) != marlin_client::GcodeTryResult::Submitted) {
        return;
    }

    queued_pos = target_pos;
}
