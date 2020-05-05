/*
 * window_dlg_wait.h
 *
 *  Created on: Nov 5, 2019
 *      Author: Migi
 */

#ifndef WINDOW_DLG_WAIT_H_
#define WINDOW_DLG_WAIT_H_

#include "window.h"

typedef struct _window_dlg_wait_t window_dlg_wait_t;

extern int16_t WINDOW_CLS_DLG_WAIT;

#pragma pack(push)
#pragma pack(1)

typedef struct _window_dlg_wait_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    font_t *font;
    font_t *font_title;
    padding_ui8_t padding;
    uint32_t timer;
    int8_t progress;
    uint16_t flags; // description in .c file
} window_dlg_wait_t;

typedef struct _window_class_dlg_wait_t {
    window_class_t cls;
} window_class_dlg_wait_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const window_class_dlg_wait_t window_class_dlg_wait;

extern int gui_dlg_wait(int8_t (*callback)());

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* WINDOW_DLG_WAIT_H_ */
