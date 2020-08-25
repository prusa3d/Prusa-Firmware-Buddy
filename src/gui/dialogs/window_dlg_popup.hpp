/*
 * window_dlg_popup.hpp
 *
 *  Created on: Nov 11, 2019
 *      Author: Migi
 */

#pragma once

#include "IDialog.hpp"
#include "window_text.hpp"

class window_dlg_popup_t : public IDialog {
    window_text_t text;

public:
    window_dlg_popup_t(Rect16 rect, string_view_utf8 txt);
};

void gui_pop_up(string_view_utf8 txt, uint32_t time = 1000);
