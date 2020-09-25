/*
 * window_dlg_popup.hpp
 *
 *  Created on: Nov 11, 2019
 *      Author: Migi
 */

#pragma once

#include "IDialog.hpp"
#include "window_text.hpp"

//Singleton dialog for messages
class window_dlg_popup_t : public IDialog {
    window_text_t text;
    uint32_t open_time;
    uint32_t ttl; //time to live

    window_dlg_popup_t(Rect16 rect, string_view_utf8 txt);
    window_dlg_popup_t(const window_dlg_popup_t &) = delete;

    void UnregisterFromParent();

private:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

public:
    //register dialog to actual screen
    static void Show(string_view_utf8 txt, uint32_t time = 1000);
};
