// screen_menu_move.cpp

#include "gui.hpp"
#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "marlin_client.h"
#include "menu_vars.h"
#include "WindowMenuItems.hpp"
#include "menu_spin_config.hpp"
#include "timing.h"

template <size_t INDEX, int8_t LONG_SEG, uint8_t BUFFER_LEN>
class MI_AXIS : public WiSpinInt {
    int32_t lastQueuedPos;
    uint32_t lastMovementOfKnob = 0;
    int32_t durationOfPause = 0;
    bool firstMove = true;

public:
    MI_AXIS<INDEX, LONG_SEG, BUFFER_LEN>()
        : WiSpinInt(int32_t(marlin_vars()->pos[INDEX]),
            SpinCnf::axis_ranges[INDEX], _(MenuVars::labels[INDEX]), 0, is_enabled_t::yes, is_hidden_t::no)
        , lastQueuedPos(int32_t(marlin_vars()->pos[INDEX])) {}

    invalidate_t Change(int diff) override {
        auto res = WiSpinInt::Change(diff);
        durationOfPause = ticks_diff(ticks_ms(), lastMovementOfKnob);
        lastMovementOfKnob = ticks_ms();
        return res;
    }

    void EnqueueNextMove() {
        marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_PQUEUE));
        if (marlin_vars()->pqueue <= BUFFER_LEN) {
            int difference = (int)value - lastQueuedPos;
            if (difference != 0) {
                float feedrate = 0;

                feedrate = MenuVars::GetManualFeedrate()[INDEX];
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
                }
                marlin_move_axis(lastQueuedPos, feedrate, INDEX);
            }
        }
    }

    virtual void OnClick() override {
        firstMove = true;
        lastMovementOfKnob = 0;
    }
};

class MI_AXIS_E : public MI_AXIS<3, 1, 1> {
public:
    virtual void OnClick() override {
        marlin_gcode("G90");    // Set to Absolute Positioning
        marlin_gcode("M82");    // Set extruder to absolute mode
        marlin_gcode("G92 E0"); // Reset position before change
        SetVal(0);              // Reset spin before change
        // original code erased invalid flag from menu. Why?
    }
};

using MI_AXIS_X = MI_AXIS<0, 5, 8>;
using MI_AXIS_Y = MI_AXIS<1, 5, 8>;
using MI_AXIS_Z = MI_AXIS<2, 1, 4>;

using Screen = ScreenMenu<EFooter::On, MI_RETURN, MI_AXIS_X, MI_AXIS_Y, MI_AXIS_Z, MI_AXIS_E>;

class ScreenMenuMove : public Screen {
public:
    constexpr static const char *label = N_("MOVE AXIS");
    ScreenMenuMove()
        : Screen(_(label)) {}

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    uint8_t enque = 0;
};

void ScreenMenuMove::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {

        bool temp_ok = (marlin_vars()->target_nozzle > MenuVars::GetExtrudeMinTemp());
        IWindowMenuItem *pAxis_E = &Item<MI_AXIS_E>();
        if (temp_ok && (!pAxis_E->IsEnabled()))
            pAxis_E->Enable();
        if ((!temp_ok) && pAxis_E->IsEnabled())
            pAxis_E->Disable();

        // enqueue next moves according to value of spinners;
        ++enque;
        if (enque == 1) {
            Item<MI_AXIS_X>().EnqueueNextMove();
            Item<MI_AXIS_Y>().EnqueueNextMove();
            Item<MI_AXIS_Z>().EnqueueNextMove();
            enque = 0;
        }
    }

    SuperWindowEvent(sender, event, param);
}

ScreenFactory::UniquePtr GetScreenMenuMove() {
    return ScreenFactory::Screen<ScreenMenuMove>();
}
