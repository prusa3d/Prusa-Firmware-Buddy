/**
 * @file screen_menu_move.cpp
 */

#include "screen_menu_move.hpp"
#include "marlin_client.h"
#include "menu_spin_config.hpp"

#include "png_resources.hpp"

I_MI_AXIS::I_MI_AXIS(size_t index)
    : WiSpinInt(int32_t(marlin_vars()->pos[index]),
        SpinCnf::axis_ranges[index], _(MenuVars::labels[index]), nullptr, is_enabled_t::yes, is_hidden_t::no)
    , lastQueuedPos(int32_t(marlin_vars()->pos[index])) {}

invalidate_t I_MI_AXIS::change(int diff) {
    auto res = WiSpinInt::change(diff);
    return res;
}

void I_MI_AXIS::loop__(size_t index, int8_t long_seg, uint8_t buffer_len) {
    marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_PQUEUE));
    if (marlin_vars()->pqueue <= buffer_len) {
        int difference = (int)value - lastQueuedPos;
        if (difference != 0) {
            float feedrate = MenuVars::GetManualFeedrate()[index];
            uint8_t freeSlots = buffer_len - marlin_vars()->pqueue;
            // move up and queue steps
            for (uint8_t i = 0; i < freeSlots && lastQueuedPos != (int)value; i++) {
                if (difference >= long_seg) {
                    lastQueuedPos += long_seg;
                    difference -= long_seg;
                } else if (difference > 0) {
                    lastQueuedPos++;
                    difference--;
                } else if (difference <= -long_seg) {
                    lastQueuedPos -= long_seg;
                    difference += long_seg;
                } else if (difference < 0) {
                    lastQueuedPos--;
                    difference++;
                }
                marlin_move_axis(lastQueuedPos, feedrate, index);
            }
        }
    }
}

void MI_AXIS_E::OnClick() {
    marlin_gcode("G90");    // Set to Absolute Positioning
    marlin_gcode("M82");    // Set extruder to absolute mode
    marlin_gcode("G92 E0"); // Reset position before change
    SetVal(0);              // Reset spin before change
    lastQueuedPos = 0;      // zero it out so we wont go back when we exit the spinner
}

void DUMMY_AXIS_E::click(IWindowMenu &window_menu) {
    marlin_gcode_printf("M1700 S E W2"); // set filament, preheat to target, return option
}

bool DUMMY_AXIS_E::IsTargetTempOk() {
    return (Filaments::Current().nozzle > 0)                                          // filament is selected
        && (int(marlin_vars()->target_nozzle + 0.9F) >= Filaments::Current().nozzle); // target temperature is high enough - +0.9 to avoid float round error
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
    return DUMMY_AXIS_E::IsTargetTempOk()                                                // target correctly set
        && (marlin_vars()->temp_nozzle > (Filaments::Current().nozzle - temp_ok_range)); // temperature nearly reached
}

ScreenMenuMove::ScreenMenuMove()
    : ScreenMenuMove__(_(label)) {
#if PRINTER_TYPE != PRINTER_PRUSA_MINI
    header.SetIcon(&png::move_16x16);
#endif
    marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_TRAVEL_ACCEL));
    prev_accel = marlin_vars()->travel_acceleration;
    marlin_gcode("M204 T200");
    Hide<MI_AXIS_E>(); // one of pair MI_AXIS_E DUMMY_AXIS_E must be hidden for swap to work
    checkNozzleTemp();
    ClrMenuTimeoutClose(); // No timeout for move screen
}

ScreenMenuMove::~ScreenMenuMove() {
    char msg[20];
    snprintf(msg, sizeof(msg), "M204 T%f", (double)prev_accel);
    marlin_gcode(msg);
}

void ScreenMenuMove::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        marlin_set_target_nozzle(0);
        marlin_set_display_nozzle(0);
        marlin_set_target_bed(0);
    }

    if (event == GUI_event_t::LOOP) {
        checkNozzleTemp();
        Item<DUMMY_AXIS_E>().Update();
    }

    SuperWindowEvent(sender, event, param);
}
