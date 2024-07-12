/**
 * @file screen_menu_move.cpp
 */

#include "screen_menu_move.hpp"
#include "screen_menu_move_utils.hpp"
#include "marlin_client.hpp"
#include "WindowMenuSpin.hpp"
#include "ScreenHandler.hpp"
#include "img_resources.hpp"
#include <option/has_toolchanger.h>
#include <config_store/store_instance.hpp>
#include <gui/menu_vars.h>

static constexpr const char *const heating_str = N_("Heating");
static constexpr const char *const low_temp_str = N_("Low temp");

xyz_float_t I_MI_AXIS::last_queued_pos {};
xyz_float_t I_MI_AXIS::target_position {};
bool I_MI_AXIS::did_final_move {};

static const NumericInputConfig &axis_ranges_spin_config(uint8_t axis) {
    // The array has to be static so that we can have references
    static std::array<NumericInputConfig, MenuVars::AXIS_CNT> data;

    // Ranges can change based on config_store, so udpate it each time
    const auto range = MenuVars::axis_range(axis);

    auto &c = data[axis];
    c = NumericInputConfig {
        .min_value = static_cast<float>(range.first),
        .max_value = static_cast<float>(range.second),
        .unit = Unit::millimeter,
    };

    return c;
}

I_MI_AXIS::I_MI_AXIS(size_t index)
    : WiSpin(round(marlin_vars().logical_curr_pos[index].get()),
        axis_ranges_spin_config(index), _(MenuVars::labels[index]), nullptr, is_enabled_t::yes, is_hidden_t::no)
    , axis_index(index) {
    xyz_float_t init_pos {
        marlin_vars().logical_curr_pos[X_AXIS].get(),
        marlin_vars().logical_curr_pos[Y_AXIS].get(),
        marlin_vars().logical_curr_pos[Z_AXIS].get(),
    };
    did_final_move = false;
    last_queued_pos = target_position = init_pos;
}

I_MI_AXIS::~I_MI_AXIS() {
    finish_move();
}

void I_MI_AXIS::Loop() {
    target_position[axis_index] = GetVal();
    jog_multiple_axis(last_queued_pos, target_position);
}

void I_MI_AXIS::finish_move() {
    if (did_final_move || last_queued_pos == target_position) {
        return;
    }

    finish_movement(last_queued_pos, target_position);
    did_final_move = true;
}

void MI_AXIS_E::OnClick() {
    marlin_client::gcode("G90"); // Set to Absolute Positioning
    marlin_client::gcode("M82"); // Set extruder to absolute mode
    marlin_client::gcode("G92 E0"); // Reset position before change
    SetVal(0); // Reset spin before change
    last_queued_position = 0; // zero it out so we wont go back when we exit the spinner
}

void MI_AXIS_E::Loop() {
    jog_axis(last_queued_position, static_cast<float>(GetVal()), AxisEnum::E_AXIS);
}

void DUMMY_AXIS_E::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::gcode("M1700 S E W2 B0"); // set filament, preheat to target, do not heat bed, return option
}

/**
 * @brief handle touch
 * it behaves the same as click, but only when extension was clicked
 */
void DUMMY_AXIS_E::touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point) {
    Rect16::Width_t width = window_menu.GetRect().Width();
    if (width >= relative_touch_point.x && (width - extension_width) <= relative_touch_point.x) {
        click(window_menu);
    }
}

bool DUMMY_AXIS_E::IsTargetTempOk() {
    const auto current_filament = config_store().get_filament_type(marlin_vars().active_extruder);
    auto current_filament_nozzle_target = filament::get_description(current_filament).nozzle_temperature;
    return (current_filament != FilamentType::none) // filament is selected
        && (int(marlin_vars().active_hotend().target_nozzle + 0.9F) >= current_filament_nozzle_target); // target temperature is high enough - +0.9 to avoid float round error
}

DUMMY_AXIS_E::DUMMY_AXIS_E()
    : WI_FORMATABLE_LABEL_t<int>(_(MenuVars::labels[MARLIN_VAR_INDEX_E]), nullptr, is_enabled_t::yes, is_hidden_t::no, 0,
        // this lambda is used during print, but does require item to be invalidated
        [&](char *buffer) {
            if (value) {
                _(heating_str).copyToRAM(buffer, GuiDefaults::infoDefaultLen);
            } else {
                _(low_temp_str).copyToRAM(buffer, GuiDefaults::infoDefaultLen);
            }
        }) {}

void DUMMY_AXIS_E::Update() {
    if (value != IsTargetTempOk()) {
        value = IsTargetTempOk();
        InValidateExtension();
    }
}

void ScreenMenuMove::checkNozzleTemp() {
#if HAS_TOOLCHANGER()
    if (!prusa_toolchanger.is_toolchanger_enabled())
    // MI_COOLDOWN is always visible on multitool
#endif /*HAS_TOOLCHANGER()*/
    {
        Item<MI_COOLDOWN>().set_is_enabled(DUMMY_AXIS_E::IsTargetTempOk());
    }

    if (IsTempOk() == Item<MI_AXIS_E>().IsHidden()) {
        menu.menu.SwapVisibility(Item<DUMMY_AXIS_E>(), Item<MI_AXIS_E>());
    }
}

// TODO make unit test
#if 0
    void checkNozzleTemp() {
        ClrMenuTimeoutClose();//dont timeout during test
        static int state  = 0;

        switch(state++) {
        case 0: // select axis e
            Hide<MI_AXIS_E>();
            Hide<MI_COOLDOWN>();
            Item<DUMMY_AXIS_E>().move_focus();
            break;
        case 1: // emulate click on e - show cooldown
            Show<MI_COOLDOWN>();
            break;
        case 2: // temp reached, enable working e, hide dummy part 0
            Show<MI_AXIS_E>();
            break;
        case 3: // temp reached, enable working e, hide dummy part 1
            if (menu.GetActiveItem() == &Item<DUMMY_AXIS_E>()) {
                Item<MI_AXIS_E>().move_focus();
            }
            break;
        case 4: // temp reached, enable working e, hide dummy part 2
            Hide<DUMMY_AXIS_E>();
            break;
        case 5: // cooldown click part 0
            Show<DUMMY_AXIS_E>();
            if (menu.GetActiveItem() == &Item<MI_AXIS_E>()) {
                Item<DUMMY_AXIS_E>().move_focus();
            }
             break;
        case 6: // cooldown click part 1
            Item<DUMMY_AXIS_E>().Update();
            break;

        case 7: // cooldown click part 2
            Hide<MI_AXIS_E>();
            break;
        case 8: // cooldown click part 3
            if (menu.GetActiveItem() == &Item<MI_COOLDOWN>()) {
                menu.Decrement(1);
            }
            Hide<MI_COOLDOWN>(); // now it is not focussed, so Hide() will succeed
            break;
        default:
            state = 0;
        }

        ClrMenuTimeoutClose();
    }
#endif // 0 .. make unit test

bool ScreenMenuMove::IsTempOk() {
    return DUMMY_AXIS_E::IsTargetTempOk() // target correctly set
        && (marlin_vars().active_hotend().temp_nozzle >= temp_ok); // Temperature is above coldextrusion
}

ScreenMenuMove::ScreenMenuMove()
    : ScreenMenuMove__(_(label)) {
#if !PRINTER_IS_PRUSA_MINI
    header.SetIcon(&img::move_16x16);
#endif
    prev_accel = marlin_vars().travel_acceleration;
    marlin_client::gcode("M9201"); // Restore default motion parameters
    marlin_client::gcode("M204 T200"); // Set accelerations
    Item<MI_AXIS_E>().set_is_hidden(true); // one of pair MI_AXIS_E DUMMY_AXIS_E must be hidden for swap to work
    checkNozzleTemp();
    ClrMenuTimeoutClose(); // No timeout for move screen
}

ScreenMenuMove::~ScreenMenuMove() {
    marlin_client::gcode_printf("M204 T%f", (double)prev_accel);
}

void ScreenMenuMove::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        HOTEND_LOOP() {
            marlin_client::set_target_nozzle(0, e);
        }
        marlin_client::set_display_nozzle(0);
        marlin_client::set_target_bed(0);
    }

    if (event == GUI_event_t::LOOP) {
        checkNozzleTemp();
        Item<DUMMY_AXIS_E>().Update();
    }

    ScreenMenu::windowEvent(sender, event, param);
}
