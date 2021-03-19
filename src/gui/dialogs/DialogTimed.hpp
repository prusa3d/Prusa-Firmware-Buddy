/**
 * @file DialogTimed.hpp
 * @author Radek Vana
 * @brief Base of dialog which opens after period without knob event and closes on knob event
 * @date 2021-03-05
 */
#pragma once

#include "IDialog.hpp"

class DialogTimed : public AddSuperWindow<IDialog> {
    const uint32_t open_period;
    uint32_t time_of_last_action;

public:
    DialogTimed(window_t *parent, Rect16 rect, uint32_t open_period);

    constexpr static bool FilterKnobEv(GUI_event_t event) {
        switch (event) {
        case GUI_event_t::CLICK:
        case GUI_event_t::ENC_DN:
        case GUI_event_t::ENC_UP:
            return true;
        default:
            return false;
        }
    }

protected:
    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param);
    bool isShowBlocked() const;
};
