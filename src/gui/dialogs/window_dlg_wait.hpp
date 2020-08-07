/*
 * window_dlg_wait.hpp
 *
 *  Created on: Nov 5, 2019
 *      Author: Migi
 */

#pragma once

#include "IDialog.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"

class window_dlg_wait_t : public IDialog {
    window_text_t text;
    window_icon_hourglass_t animation;

public:
    window_dlg_wait_t(rect_ui16_t rect);
};

#define DLG_W8_DRAW_HOURGLASS 0x04 // Draw hourglass animation
#define DLG_W8_DRAW_FRAME     0x01 // Draw grey frame
#define DLG_W8_DRAW_PROGRESS  0x02 // Draw progress bar

/*!*********************************************************************************************************************
* \brief GUI dialog for processes that require user to wait calmly.
*
* \param [in] progress_callback - function callback that returns current progress
*
* \param [in] comp_flag - stores which compoments to draw (DLG_W8_DRAW_HOURGLASS | DLG_W8_DRAW_FRANE | DLG_W8_DRAW_PROGRESS)
*
* It creates inner gui_loop cycle that keeps GUI running while waiting.
*/
void gui_dlg_wait(void (*closing_callback)());
