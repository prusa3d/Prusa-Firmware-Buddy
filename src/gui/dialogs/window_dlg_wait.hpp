/*
 * window_dlg_wait.hpp
 *
 *  Created on: Nov 5, 2019
 *      Author: Migi
 */

#pragma once

#include "IDialog.hpp"
#include "string_view_utf8.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"

class window_dlg_wait_t : public IDialog {
    window_text_t text;
    window_text_t second_text;
    window_icon_hourglass_t animation;

public:
    window_dlg_wait_t(Rect16 rect, string_view_utf8 second_string = string_view_utf8::MakeNULLSTR());
};

/*!*********************************************************************************************************************
 * \brief GUI dialog for processes that require user to wait calmly.
 *
 * \param [in] progress_callback - function callback that returns current progress
 *
 * It creates inner gui_loop cycle that keeps GUI running while waiting.
 */
void gui_dlg_wait(std::function<void()> closing_callback, string_view_utf8 second_string = string_view_utf8::MakeNULLSTR());
