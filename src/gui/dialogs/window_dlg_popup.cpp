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

window_dlg_popup_t::window_dlg_popup_t(Rect16 rect, string_view_utf8 txt)
    : IDialog(rect)
    , text(this, rect, is_multiline::yes, is_closed_on_click_t::no, txt)
    , open_time(0)
    , ttl(0) {
    text.SetAlignment(ALIGN_LEFT_TOP);
    text.SetPadding({ 0, 2, 0, 2 });
}

void window_dlg_popup_t::Show(string_view_utf8 txt, uint32_t time) {
    static window_dlg_popup_t dlg(Rect16(0, 32, 240, 120), txt);
    dlg.open_time = HAL_GetTick();
    dlg.ttl = time;
    dlg.text.SetText(txt);
    //in 1st call text will be set twice, I could use static bool variable to prevent it
    //but i prefer fewer code instead
}

//no need to care about focus/caption after unregistration
//Screens::Loop() auto sets focus and caption to new screen or its child window
void window_dlg_popup_t::UnregisterFromParent() {
    if (!GetParent())
        return;
    GetParent()->UnregisterSubWin(this);
}

void window_dlg_popup_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    const uint32_t openned = HAL_GetTick() - open_time;
    if (openned > ttl)
        UnregisterFromParent();
    IDialog::windowEvent(sender, event, param);
}
/*
void gui_pop_up(string_view_utf8 txt, uint32_t time) {
    window_dlg_popup_t dlg(Rect16(0, 32, 240, 120), txt);
    const uint32_t open_time = HAL_GetTick();
    // create lambda function, convert it to std::function<void(uint32_t, uint32_t)>
    // and pass it to dlg.MakeBlocking
    dlg.MakeBlocking(
        (std::function<void(uint32_t, uint32_t)>)[](uint32_t open_time_, uint32_t time_) {
            if ((int(HAL_GetTick()) - int(open_time_)) > int(time_))
                Screens::Access()->Close();
        },
        open_time, time);
}
*/
//vyresit capture / focus, kdyz se zavre parrent destruktor frame ?
