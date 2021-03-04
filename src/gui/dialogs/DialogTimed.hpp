/**
 * @file DialogTimed.hpp
 * @author Radek Vana
 * @brief Base of dialog which opens after period without knob event and closes on knob event
 * @date 2021-03-05
 */
#pragma once

#include "IDialog.hpp"

class TimedDialog : public AddSuperWindow<IDialog> {
    const uint32_t open_period;
    uint32_t time_of_last_action;

public:
    TimedDialog(Rect16 rect, uint32_t open_period);

    void Tick(window_t &parent, GUI_event_t event);

protected:
    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param);
};
