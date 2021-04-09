/*
 * window_dlg_popup.cpp
 *
 *  Created on: Nov 11, 2019
 *      Author: Migi
 */

#include "window_dlg_popup.hpp"
#include "ScreenHandler.hpp"
#include "cmsis_os.h" //HAL_GetTick

window_dlg_popup_t::window_dlg_popup_t(Rect16 rect, string_view_utf8 txt)
    : AddSuperWindow<window_frame_t>(Screens::Access()->Get(), rect, win_type_t::popup)
    , text(this, rect, is_multiline::yes, is_closed_on_click_t::no, txt)
    , open_time(0)
    , ttl(0) {
    Disable();
    text.SetAlignment(Align_t::LeftTop());
    text.SetPadding({ 0, 2, 0, 2 });
}

void window_dlg_popup_t::Show(Rect16 rect, string_view_utf8 txt, uint32_t time) {
    static window_dlg_popup_t dlg(rect, txt);
    dlg.open_time = HAL_GetTick();
    dlg.ttl = time;
    dlg.text.SetText(txt);
    dlg.rect = rect;
    if (!dlg.GetParent()) {
        window_t *parent = Screens::Access()->Get();
        if (parent) {
            parent->RegisterSubWin(dlg);
        }
    }
}

void window_dlg_popup_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    const uint32_t openned = HAL_GetTick() - open_time;
    if (event == GUI_event_t::LOOP && openned > ttl) { //todo use timer
        if (GetParent()) {
            GetParent()->UnregisterSubWin(*this);
            //frame will set parrent to null
        }
    } else
        SuperWindowEvent(sender, event, param);
}
