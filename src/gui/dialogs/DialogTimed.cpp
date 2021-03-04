/**
 * @file DialogTimed.cpp
 * @author Radek Vana
 * @date 2021-03-05
 */

#include "DialogTimed.hpp"
#include "stm32f4xx_hal.h" //HAL_GetTick

TimedDialog::TimedDialog(Rect16 rect, uint32_t open_period)
    : AddSuperWindow<IDialog>(rect)
    , open_period(open_period)
    , time_of_last_action(HAL_GetTick()) {}

void TimedDialog::Tick(window_t &parent, GUI_event_t event) {

    if (GetParent() == nullptr) {
        //closed
        uint32_t now = HAL_GetTick();

        switch (event) {
        case GUI_event_t::CLICK:
        case GUI_event_t::ENC_DN:
        case GUI_event_t::ENC_UP:
            //refresh
            time_of_last_action = now;
            return;
        default:
            break;
        }

        if (now - time_of_last_action >= open_period) {
            //open
            parent.RegisterSubWin(this);
        }
    }
}

void TimedDialog::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    //openned

    switch (event) {
    case GUI_event_t::CLICK:
    case GUI_event_t::ENC_DN:
    case GUI_event_t::ENC_UP:
        //close
        GetParent()->UnregisterSubWin(this);
        time_of_last_action = HAL_GetTick();
    default:
        break;
    }
}
