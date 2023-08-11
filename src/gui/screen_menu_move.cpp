/**
 * @file screen_menu_move.cpp
 */

#include "screen_menu_move.hpp"
#include "marlin_client.hpp"
#include "menu_spin_config.hpp"

#include "img_resources.hpp"
#include <config_store/store_instance.hpp>

I_MI_AXIS::I_MI_AXIS(size_t index)
    : WiSpinInt(round(marlin_vars()->logical_pos[index]),
        SpinCnf::axis_ranges[index], _(MenuVars::labels[index]), nullptr, is_enabled_t::yes, is_hidden_t::no)
    , last_queued_position(value.i) {}

void I_MI_AXIS::loop__(size_t axis) {
    if ((int)value == (int)last_queued_position) {
        // Yeah and every time I try to go where I really want to be
        // it's already where I am 'cause I'm already there!
        return;
    }

    // This empirical constant was carefully crafted in such
    // a clever way that it seems to work most of the time.
    constexpr float magic_constant = 1. / (BLOCK_BUFFER_SIZE * 60 * 1.25 * 5);
    const float feedrate = MenuVars::GetManualFeedrate()[axis];
    const float short_segment = feedrate * magic_constant;
    const float long_segment = 5 * short_segment;

    // Just fill the entire queue with movements.
    for (uint8_t i = marlin_vars()->pqueue; i < BLOCK_BUFFER_SIZE; i++) {
        const float difference = (int)value - last_queued_position;
        if (difference == 0) {
            break;
        } else if (difference >= long_segment) {
            last_queued_position += long_segment;
        } else if (difference >= short_segment) {
            last_queued_position += short_segment;
        } else if (difference <= -long_segment) {
            last_queued_position -= long_segment;
        } else if (difference <= -short_segment) {
            last_queued_position -= short_segment;
        } else {
            last_queued_position = int(value);
        }
        marlin_client::move_axis(last_queued_position, feedrate, axis);
    }
}

void MI_AXIS_E::OnClick() {
    marlin_client::gcode("G90");    // Set to Absolute Positioning
    marlin_client::gcode("M82");    // Set extruder to absolute mode
    marlin_client::gcode("G92 E0"); // Reset position before change
    SetVal(0);                      // Reset spin before change
    last_queued_position = 0;       // zero it out so we wont go back when we exit the spinner
}

void DUMMY_AXIS_E::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::gcode_printf("M1700 S E W2"); // set filament, preheat to target, return option
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
    auto current_filament = config_store().get_filament_type(marlin_vars()->active_extruder);
    auto current_filament_nozzle_target = filament::get_description(current_filament).nozzle;
    return (current_filament != filament::Type::NONE)                                                    // filament is selected
        && (int(marlin_vars()->active_hotend().target_nozzle + 0.9F) >= current_filament_nozzle_target); // target temperature is high enough - +0.9 to avoid float round error
}

DUMMY_AXIS_E::DUMMY_AXIS_E()
    : WI_FORMATABLE_LABEL_t<int>(_(MenuVars::labels[MARLIN_VAR_INDEX_E]), nullptr, is_enabled_t::yes, is_hidden_t::no, 0,
        // this lambda is used during print, but does require item to be invalidated
        [&](char *buffer) {
            if (value) {
                snprintf(buffer, GuiDefaults::infoDefaultLen, N_("Heating"));
            } else {
                snprintf(buffer, GuiDefaults::infoDefaultLen, N_("Low temp"));
            }
        }) {}

void DUMMY_AXIS_E::Update() {
    if (value != IsTargetTempOk()) {
        value = IsTargetTempOk();
        InValidateExtension();
    }
}

void ScreenMenuMove::checkNozzleTemp() {
    DUMMY_AXIS_E::IsTargetTempOk() ? EnableItem<MI_COOLDOWN>() : DisableItem<MI_COOLDOWN>();

    if (IsTempOk() == Item<MI_AXIS_E>().IsHidden()) {
        menu.SwapVisibility(Item<DUMMY_AXIS_E>(), Item<MI_AXIS_E>());
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
            menu.SetActiveItem(Item<DUMMY_AXIS_E>());
            break;
        case 1: // emulate click on e - show cooldown
            Show<MI_COOLDOWN>();
            break;
        case 2: // temp reached, enable working e, hide dummy part 0
            Show<MI_AXIS_E>();
            break;
        case 3: // temp reached, enable working e, hide dummy part 1
            if (menu.GetActiveItem() == &Item<DUMMY_AXIS_E>()) {
                menu.SetActiveItem(Item<MI_AXIS_E>());
            }
            break;
        case 4: // temp reached, enable working e, hide dummy part 2
            Hide<DUMMY_AXIS_E>();
            break;
        case 5: // cooldown click part 0
            Show<DUMMY_AXIS_E>();
            if (menu.GetActiveItem() == &Item<MI_AXIS_E>()) {
                menu.SetActiveItem(Item<DUMMY_AXIS_E>());
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
    auto current_filament = config_store().get_filament_type(marlin_vars()->active_extruder);
    auto current_filament_nozzle_target = filament::get_description(current_filament).nozzle;
    return DUMMY_AXIS_E::IsTargetTempOk()                                                                   // target correctly set
        && (marlin_vars()->active_hotend().temp_nozzle > (current_filament_nozzle_target - temp_ok_range)); // temperature nearly reached
}

ScreenMenuMove::ScreenMenuMove()
    : ScreenMenuMove__(_(label)) {
#if !PRINTER_IS_PRUSA_MINI
    header.SetIcon(&img::move_16x16);
#endif
    prev_accel = marlin_vars()->travel_acceleration;
    marlin_client::gcode("M204 T200");
    Hide<MI_AXIS_E>();     // one of pair MI_AXIS_E DUMMY_AXIS_E must be hidden for swap to work
    checkNozzleTemp();
    ClrMenuTimeoutClose(); // No timeout for move screen
}

ScreenMenuMove::~ScreenMenuMove() {
    char msg[20];
    snprintf(msg, sizeof(msg), "M204 T%f", (double)prev_accel);
    marlin_client::gcode(msg);
}

void ScreenMenuMove::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        marlin_client::set_target_nozzle(0);
        marlin_client::set_display_nozzle(0);
        marlin_client::set_target_bed(0);
    }

    if (event == GUI_event_t::LOOP) {
        checkNozzleTemp();
        Item<DUMMY_AXIS_E>().Update();
    }

    SuperWindowEvent(sender, event, param);
}
