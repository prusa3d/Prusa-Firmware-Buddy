/**
 * @file DialogTimed.cpp
 * @author Radek Vana
 * @date 2021-03-05
 */

#include "DialogTimed.hpp"
//#include "stm32f4xx_hal.h" //HAL_GetTick
#include "cmsis_os.h" //HAL_GetTick

DialogTimed::DialogTimed(window_t *parent, Rect16 rect, uint32_t open_period)
    : AddSuperWindow<IDialog>(parent, rect)
    , open_period(open_period)
    , time_of_last_action(HAL_GetTick()) {
    Hide(); //default behavior of this dialog is hidden
}

void DialogTimed::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {

    // must have parrent, could crash without it
    if (!GetParent()) {
        return;
    }

    uint32_t now = HAL_GetTick();

    // IsVisible would not work as expected when it is hidden behind dialog
    if (IsHiddenBehindDialog()) {
        Hide(); // Hide will notify parent, and parent will clear HiddenBehindDialog flag
        time_of_last_action = now;
        return;
    }

    if (IsVisible()) {
        if (GUI_event_IsCaptureEv(event)) { // this window must be captured
            Hide();
            time_of_last_action = now;
            return; // event consumed
        }

    } else { // not visible

        //Reset timeout
        if (GUI_event_IsKnob(event) // knob events sent to all windows
            || isShowBlocked()) {
            time_of_last_action = now;
            return; // event consumed
        }

        if (now - time_of_last_action >= open_period) {
            Show();
            return; // event consumed
        }
    }

    //resend capture events
    if (GUI_event_IsCaptureEv(event)) {
        DoNotEnforceCapture_ScopeLock Lock(*this); // avoid resend event to itself
        window_t *captured = GetParent()->GetCapturedWindow();
        if (captured) {
            captured->WindowEvent(sender, event, param);
        }
    }
}

bool DialogTimed::isShowBlocked() const {
    if (!GetParent())
        return true;
    WinFilterVisible filter;
    window_t *begin;
    window_t *end;

    if (GetParent()->GetFirstDialog() && GetParent()->GetLastDialog()) {
        begin = GetParent()->GetFirstDialog();
        end = GetParent()->GetLastDialog()->GetNext();
        if (findFirst(begin, end, filter) != end)
            return true;
    }

    if (GetParent()->GetFirstStrongDialog() && GetParent()->GetLastStrongDialog()) {
        begin = GetParent()->GetFirstStrongDialog();
        end = GetParent()->GetLastStrongDialog()->GetNext();
        if (findFirst(begin, end, filter) != end)
            return true;
    }

    return false;
}
