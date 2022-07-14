// screen_menu_move.cpp

#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "marlin_client.h"
#include "WindowMenuItems.hpp"
#include "menu_spin_config.hpp"
#include "WindowItemFormatableLabel.hpp"
#include "window_dlg_load_unload.hpp"
#include "MItem_filament.hpp"

template <size_t INDEX, int8_t LONG_SEG, uint8_t BUFFER_LEN>
class MI_AXIS : public WiSpinInt {
protected:
    int32_t lastQueuedPos;

    invalidate_t change(int diff) override {
        auto res = WiSpinInt::change(diff);
        return res;
    }

public:
    MI_AXIS<INDEX, LONG_SEG, BUFFER_LEN>()
        : WiSpinInt(int32_t(marlin_vars()->pos[INDEX]),
            SpinCnf::axis_ranges[INDEX], _(MenuVars::labels[INDEX]), 0, is_enabled_t::yes, is_hidden_t::no)
        , lastQueuedPos(int32_t(marlin_vars()->pos[INDEX])) {}

    // enqueue next moves according to value of spinners;
    virtual void Loop() override {
        marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_PQUEUE));
        if (marlin_vars()->pqueue <= BUFFER_LEN) {
            int difference = (int)value - lastQueuedPos;
            if (difference != 0) {
                float feedrate = MenuVars::GetManualFeedrate()[INDEX];
                uint8_t freeSlots = BUFFER_LEN - marlin_vars()->pqueue;
                // move up and queue steps
                for (uint8_t i = 0; i < freeSlots && lastQueuedPos != (int)value; i++) {
                    if (difference >= LONG_SEG) {
                        lastQueuedPos += LONG_SEG;
                        difference -= LONG_SEG;
                    } else if (difference > 0) {
                        lastQueuedPos++;
                        difference--;
                    } else if (difference <= -LONG_SEG) {
                        lastQueuedPos -= LONG_SEG;
                        difference += LONG_SEG;
                    } else if (difference < 0) {
                        lastQueuedPos--;
                        difference++;
                    }
                    marlin_move_axis(lastQueuedPos, feedrate, INDEX);
                }
            }
        }
    }
};

class MI_AXIS_E : public MI_AXIS<3, 5, 8> {

public:
    MI_AXIS_E()
        : MI_AXIS<3, 5, 8>() {}

    virtual void OnClick() override {
        marlin_gcode("G90");    // Set to Absolute Positioning
        marlin_gcode("M82");    // Set extruder to absolute mode
        marlin_gcode("G92 E0"); // Reset position before change
        SetVal(0);              // Reset spin before change
        lastQueuedPos = 0;      // zero it out so we wont go back when we exit the spinner
    }
};

class DUMMY_AXIS_E : public WI_FORMATABLE_LABEL_t<int> {
    virtual void click(IWindowMenu &window_menu) override {
        marlin_gcode_printf("M1700 S E W2"); // set filament, preheat to target, return option
    }

public:
    static bool IsTargetTempOk() {
        return (Filaments::Current().nozzle > 0)                                          // filament is selected
            && (int(marlin_vars()->target_nozzle + 0.9F) >= Filaments::Current().nozzle); // target temperature is high enough - +0.9 to avoid float round error
    }
    DUMMY_AXIS_E()
        : WI_FORMATABLE_LABEL_t<int>(_(MenuVars::labels[MARLIN_VAR_INDEX_E]), IsTargetTempOk(), is_enabled_t::yes, is_hidden_t::no, 0,
            // this lambda is used during print, but does require item to be invalidated
            [&](char *buffer) {
                if (value) {
                    snprintf(buffer, GuiDefaults::infoDefaultLen, N_("Heating"));
                } else {
                    snprintf(buffer, GuiDefaults::infoDefaultLen, N_("Low temp"));
                }
            }) {}

    // TODO call automatically in men loop
    void Update() {
        if (value != IsTargetTempOk()) {
            value = IsTargetTempOk();
            InValidateExtension();
        }
    }
};

using MI_AXIS_X = MI_AXIS<0, 5, 8>;
using MI_AXIS_Y = MI_AXIS<1, 5, 8>;
using MI_AXIS_Z = MI_AXIS<2, 1, 4>;

using Screen = ScreenMenu<EFooter::On, MI_RETURN, MI_AXIS_X, MI_AXIS_Y, MI_AXIS_Z, MI_AXIS_E, DUMMY_AXIS_E, MI_COOLDOWN>;

class ScreenMenuMove : public Screen {
    float prev_accel;

    void checkNozzleTemp() {
        bool can_timeout = true;

        if (DUMMY_AXIS_E::IsTargetTempOk()) {
            Show<MI_COOLDOWN>();
            can_timeout = false; //just do not timeout whe we are heating
        } else {
            if (Item<MI_COOLDOWN>().IsFocused()) {
                menu.Decrement(1);
            }
            Hide<MI_COOLDOWN>(); // now it is not focussed, so Hide() will succeed
        }

        if (IsTempOk()) {
            Show<MI_AXIS_E>();
            if (Item<DUMMY_AXIS_E>().IsFocused()) {
                menu.Decrement(1); //set focus to previous (MI_AXIS_E)
            }
            Hide<DUMMY_AXIS_E>();
        } else {
            Show<DUMMY_AXIS_E>();
            if (Item<MI_AXIS_E>().IsFocused()) {
                menu.Increment(1); //set focus to next (DUMMY_AXIS_E)
            }
            Hide<MI_AXIS_E>();
            Item<DUMMY_AXIS_E>().Update();
        }

        can_timeout ? SetMenuTimeoutClose() : ClrMenuTimeoutClose();
    }

public:
    constexpr static const char *label = N_("MOVE AXIS");
    static constexpr int temp_ok_range = 10;
    static bool IsTempOk() {
        return DUMMY_AXIS_E::IsTargetTempOk()                                                // target correctly set
            && (marlin_vars()->temp_nozzle > (Filaments::Current().nozzle - temp_ok_range)); // temperature nearly reached
    }

    ScreenMenuMove()
        : Screen(_(label)) {
        marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_TRAVEL_ACCEL));
        prev_accel = marlin_vars()->travel_acceleration;
        marlin_gcode("M204 T200");
        checkNozzleTemp();
    }
    ~ScreenMenuMove() {
        char msg[20];
        snprintf(msg, sizeof(msg), "M204 T%f", (double)prev_accel);
        marlin_gcode(msg);
    }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

void ScreenMenuMove::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        marlin_set_target_nozzle(0);
        marlin_set_display_nozzle(0);
        marlin_set_target_bed(0);
        return;
    }

    if (event == GUI_event_t::LOOP) {
        checkNozzleTemp();
    }

    SuperWindowEvent(sender, event, param);
}

ScreenFactory::UniquePtr GetScreenMenuMove() {
    return ScreenFactory::Screen<ScreenMenuMove>();
}
