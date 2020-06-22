// window_text.h

#ifndef _WINDOW_TEXT_H
#define _WINDOW_TEXT_H

#include "window.h"
#include "str_utils.h"

typedef struct _window_class_text_t {
    window_class_t cls;
} window_class_text_t;

typedef struct _window_text_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    font_t *font;
    char *text;
    padding_ui8_t padding;
    uint8_t alignment;
    ml_data_t *pml_data;
} window_text_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const window_class_text_t window_class_text;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WINDOW_TEXT_H
