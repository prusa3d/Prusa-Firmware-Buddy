/*
 * window_dlg_wait.hpp
 *
 *  Created on: Nov 5, 2019
 *      Author: Migi
 */

#pragma once

#include "window.hpp"
#include <stdbool.h>

extern int16_t WINDOW_CLS_DLG_WAIT;

struct window_dlg_wait_t : public window_t {
    color_t color_text;
    font_t *font;
    font_t *font_title;
    padding_ui8_t padding;
    uint32_t timer;
    int8_t progress;
    uint8_t animation;
    uint8_t components;
    bool animation_chng;
    bool progress_chng;
    window_dlg_wait_t(window_t *parent, rect_ui16_t rect);
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
extern void gui_dlg_wait(int8_t (*progress_callback)(), uint8_t comp_flag);
