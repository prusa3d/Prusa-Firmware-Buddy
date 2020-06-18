// window_numb.h

#ifndef _WINDOW_NUMB_H
#define _WINDOW_NUMB_H

#include "window.h"

#define WINDOW_FLG_NUMB_FLOAT2INT (WINDOW_FLG_USER << 1)

typedef struct _window_class_numb_t {
    window_class_t cls;
} window_class_numb_t;

typedef struct _window_numb_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    font_t *font;
    float value;
    const char *format;
    padding_ui8_t padding;
    uint8_t alignment;
} window_numb_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const window_class_numb_t window_class_numb;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WINDOW_NUMB_H
