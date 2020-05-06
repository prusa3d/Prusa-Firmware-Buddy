/*  window_roll_text.h
*   \brief used in texts that are too long for standart display width
*
*  Created on: May 6, 2020
*      Author: Migi - michal.rudolf<at>prusa3d.cz
*/

#ifndef WINDOW_ROLL_TEXT_H
#define WINDOW_ROLL_TEXT_H

#include "window.h"
#include "display_helper.h"

typedef struct _window_class_roll_text_t {
    window_class_t cls;
} window_class_roll_text_t;

typedef struct _window_roll_text_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    font_t *font;
    char *text;
    padding_ui8_t padding;
    uint8_t alignment;
    txtroll_t roll;
} window_roll_text_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const window_class_roll_text_t window_class_roll_text;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //WINDOW_SCROLL_TEXT_H
