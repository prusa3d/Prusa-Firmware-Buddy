/*
 * window_dlg_popup.cpp
 *
 *  Created on: Nov 11, 2019
 *      Author: Migi
 */

#include "window_dlg_popup.hpp"
#include "ScreenHandler.hpp"

window_dlg_popup_t::window_dlg_popup_t(Rect16 rect, const string_view_utf8 &txt)
    : window_frame_t(Screens::Access()->Get(), rect, win_type_t::popup)
    , text(this, rect, is_multiline::yes, is_closed_on_click_t::no, txt)
    , open_time(0)
    , ttl(0) {
    Disable();
    text.SetAlignment(Align_t::LeftTop());
    text.SetPadding({ 0, 2, 0, 2 });
}

void window_dlg_popup_t::Show(Rect16 rect, const string_view_utf8 &txt, uint32_t time) {
    static window_dlg_popup_t dlg(rect, txt);

    window_t *parent = Screens::Access()->Get();

    // hide the dialog if shown already (it is static)
    if (dlg.GetParent() && dlg.GetParent() != parent) {
        dlg.GetParent()->UnregisterSubWin(dlg);
    }

    dlg.open_time = gui::GetTick();
    dlg.ttl = time;
    dlg.text.SetText(txt);
    dlg.text.Invalidate(); // invalidation is needed here because we are using the same static array for the text and text will invalidate only when the memory address is different
    dlg.SetRect(rect);

    if (!dlg.GetParent() && parent) {
        parent->RegisterSubWin(dlg);
    }
}

void window_dlg_popup_t::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    const uint32_t opened = gui::GetTick() - open_time;
    if (event == GUI_event_t::LOOP && opened > ttl) { // todo use timer
        if (GetParent()) {
            GetParent()->UnregisterSubWin(*this);
            // frame will set parrent to null
        }
    } else {
        window_frame_t::windowEvent(sender, event, param);
    }
}
