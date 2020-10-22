/*
 * window_dlg_popup.cpp
 *
 *  Created on: Nov 11, 2019
 *      Author: Migi
 */

#include "window_dlg_popup.hpp"
#include "display_helper.h"
#include "i18n.h"
#include "ScreenHandler.hpp"

window_dlg_popup_t::window_dlg_popup_t(Rect16 rect, string_view_utf8 txt, SetCapture_t setCapture)
    : AddSuperWindow<IDialog>(rect, setCapture)
    , text(this, rect, is_multiline::yes, is_closed_on_click_t::no, txt)
    , open_time(0)
    , ttl(0) {
    text.SetAlignment(ALIGN_LEFT_TOP);
    text.SetPadding({ 0, 2, 0, 2 });
}

void window_dlg_popup_t::Show(string_view_utf8 txt, uint32_t time) {
    static window_dlg_popup_t dlg(Rect16(0, 70, 240, 120), txt, SetCapture_t::no);
    dlg.open_time = HAL_GetTick();
    dlg.ttl = time;
    dlg.text.SetText(txt);
    if (!dlg.GetParent()) {
        window_t *parent = Screens::Access()->Get();
        if (parent) {
            dlg.SetParent(parent);
            parent->RegisterSubWin(&dlg);
        }
    }

    //DO NOT erase this commented code, might be still used
    /*if (GetCapturedWindow() != &dlg) {
        dlg.StoreCapture();
        dlg.SetCapture();
    }*/
}

//no need to care about focus/caption after unregistration
//Screens::Loop() auto sets focus and caption to new screen or its child window
void window_dlg_popup_t::UnregisterFromParent() {
    if (!GetParent())
        return;
    //DO NOT erase this commented code, might be still used
    //releaseCapture();
    GetParent()->UnregisterSubWin(this);
    SetParent(nullptr);
}

void window_dlg_popup_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    const uint32_t openned = HAL_GetTick() - open_time;
    if (event == GUI_event_t::LOOP && openned > ttl) //todo use timer
        UnregisterFromParent();
    SuperWindowEvent(sender, event, param);
}
