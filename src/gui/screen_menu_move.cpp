// screen_menu_move.cpp

#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "marlin_client.h"
#include "WindowMenuItems.hpp"
#include "menu_spin_config.hpp"
#include "WindowItemFormatableLabel.hpp"
#include "window_dlg_load_unload.hpp"

template <size_t INDEX, int8_t LONG_SEG, uint8_t BUFFER_LEN>
class MI_AXIS : public WiSpinInt {
protected:
    int32_t lastQueuedPos;

public:
    MI_AXIS<INDEX, LONG_SEG, BUFFER_LEN>()
        : WiSpinInt(int32_t(marlin_vars()->pos[INDEX]),
            SpinCnf::axis_ranges[INDEX], _(MenuVars::labels[INDEX]), 0, is_enabled_t::yes, is_hidden_t::no)
        , lastQueuedPos(int32_t(marlin_vars()->pos[INDEX])) {}

    invalidate_t Change(int diff) override {
        auto res = WiSpinInt::Change(diff);
        return res;
    }

    void EnqueueNextMove() {
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
        PreheatStatus::Dialog(PreheatMode::None, RetAndCool_t::Both);
    }

public:
    DUMMY_AXIS_E()
        : WI_FORMATABLE_LABEL_t<int>(_(MenuVars::labels[MARLIN_VAR_INDEX_E]), 0, is_enabled_t::yes, is_hidden_t::no, 0, [&](char *buffer) {
            if (marlin_vars()->target_nozzle < MenuVars::GetExtrudeMinTemp()) {
                snprintf(buffer, GuiDefaults::infoMaxLen, "Low temp");
            } else {
                snprintf(buffer, GuiDefaults::infoMaxLen, "Heating");
            }
        }) {}
};

using MI_AXIS_X = MI_AXIS<0, 5, 8>;
using MI_AXIS_Y = MI_AXIS<1, 5, 8>;
using MI_AXIS_Z = MI_AXIS<2, 1, 4>;

using Screen = ScreenMenu<EFooter::On, MI_RETURN, MI_AXIS_X, MI_AXIS_Y, MI_AXIS_Z, MI_AXIS_E, DUMMY_AXIS_E>;

class ScreenMenuMove : public Screen {
    float prev_accel;

    void CheckNozzleTemp() {
        MI_AXIS_E *spinner = &Item<MI_AXIS_E>();
        DUMMY_AXIS_E *dummy = &Item<DUMMY_AXIS_E>();
        bool temp_ok = (marlin_vars()->temp_nozzle > MenuVars::GetExtrudeMinTemp());
        spinner->SetVisibility(temp_ok);
        dummy->SetVisibility(!temp_ok);
    }

public:
    constexpr static const char *label = N_("MOVE AXIS");
    ScreenMenuMove()
        : Screen(_(label)) {
        marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_TRAVEL_ACCEL));
        prev_accel = marlin_vars()->travel_acceleration;
        marlin_gcode("M204 T200");
        CheckNozzleTemp();
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
    if (event == GUI_event_t::LOOP) {
        // enqueue next moves according to value of spinners;
        Item<MI_AXIS_X>().EnqueueNextMove();
        Item<MI_AXIS_Y>().EnqueueNextMove();
        Item<MI_AXIS_Z>().EnqueueNextMove();
        Item<MI_AXIS_E>().EnqueueNextMove();

        CheckNozzleTemp();
    }

    SuperWindowEvent(sender, event, param);
}

ScreenFactory::UniquePtr GetScreenMenuMove() {
    return ScreenFactory::Screen<ScreenMenuMove>();
}
