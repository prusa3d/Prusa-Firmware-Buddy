/**
 * @file screen.cpp
 * @author Radek Vana
 * @date 2020-11-29
 */

#include "screen.hpp"

screen_t::screen_t(window_t *parent, win_type_t type, is_closed_on_timeout_t timeout, is_closed_on_printing_t close_on_print)
    : window_frame_t(parent, GuiDefaults::RectScreen, type, timeout, close_on_print) {}

bool screen_t::registerSubWin(window_t &win) {
    switch (win.GetType()) {
    case win_type_t::normal:
        registerAnySubWin(win, first_normal, last_normal);
        break;
    case win_type_t::dialog:
        registerAnySubWin(win, first_dialog, last_dialog);
        if (&win == first_dialog && last_normal) { // connect to list
            last_normal->SetNext(&win);
        }
        break;
    default:
        return false;
    }

    clearAllHiddenBehindDialogFlags();
    hideSubwinsBehindDialogs();
    return true;
}

void screen_t::hideSubwinsBehindDialogs() {
    if ((!first_normal) || (!last_normal)) {
        return; // error, must have normal window
    }
    window_t *pBeginAbnormal = nullptr;
    if (first_dialog) {
        pBeginAbnormal = first_dialog;
    }
    if (!pBeginAbnormal) {
        return; // nothing to hide
    }
    window_t *pEndAbnormal = nullptr;

    // find last_normal visible dialog
    WinFilterVisible filter_visible;
    window_t *pLastVisibleDialog;
    while ((pLastVisibleDialog = findLast(pBeginAbnormal, pEndAbnormal, filter_visible)) != pEndAbnormal) {
        // hide all conflicting windows
        WinFilterIntersectingVisible filter_intersecting(pLastVisibleDialog->GetRect());
        window_t *pIntersectingWin;
        while ((pIntersectingWin = findFirst(first_normal, pLastVisibleDialog, filter_intersecting)) != pLastVisibleDialog) {
            pIntersectingWin->HideBehindDialog();
        }

        pEndAbnormal = pLastVisibleDialog; // new end of search
    }
}

void screen_t::unregisterSubWin(window_t &win) {
    switch (win.GetType()) {
    case win_type_t::normal:
        unregisterAnySubWin(win, first_normal, last_normal);
        return; // return - normal window does not affect other windows
    case win_type_t::dialog:
        unregisterAnySubWin(win, first_dialog, last_dialog);
        break;
    }

    clearAllHiddenBehindDialogFlags();
    hideSubwinsBehindDialogs();
}

window_t *screen_t::get_child_dialog(ChildDialogParam param) const {
    switch (param) {
    case ChildDialogParam::first_dialog:
        return first_dialog;
    case ChildDialogParam::last_dialog:
        return last_dialog;
    }

    return nullptr;
}

window_t *screen_t::GetCapturedWindow() {
    window_t *ret;

    ret = findCaptured_first_last(first_dialog, last_dialog);
    if (ret) {
        return ret;
    }

    // default frame behavior
    return window_frame_t::GetCapturedWindow();
}

window_t *screen_t::findCaptured_first_last(window_t *first, window_t *last) const {
    if ((!first) || (!last)) {
        return nullptr;
    }

    // last can be directly accessed
    if (last->IsCapturable()) {
        return last->GetCapturedWindow();
    }

    // non last can not be directly accessed
    WinFilterCapturable filter;
    window_t *win = findLast(first, last, filter);
    if (win != last) {
        return win;
    }

    return nullptr;
}

void screen_t::ChildVisibilityChanged(window_t &child) {
    window_frame_t::ChildVisibilityChanged(child);
    clearAllHiddenBehindDialogFlags();
    hideSubwinsBehindDialogs();
}

void screen_t::screenEvent(window_t *sender, GUI_event_t event, void *param) {
    // GUI touch events (SWIPE) are to be distributed only to the first active dialog
    if (GUI_event_is_touch_event(event)) {
        const auto checkList = [&](window_t *start, window_t *end) {
            for (auto w = start; w; w = w->GetNext()) {
                if (w->IsVisible()) {
                    w->ScreenEvent(sender, event, param);
                    return true;
                }

                // Has to be checked at the end of the loop, cannot be in the for condition
                if (w == end) {
                    break;
                }
            }

            return false;
        };

        if (checkList(GetFirstDialog(), GetLastDialog())) {
            return;
        }
    }

    window_frame_t::screenEvent(sender, event, param);
}
