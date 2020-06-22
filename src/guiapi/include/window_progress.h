// window_progress.h

#ifndef _WINDOW_PROGRESS_H
#define _WINDOW_PROGRESS_H

#include "window.h"

typedef struct _window_class_progress_t {
    window_class_t cls;
} window_class_progress_t;

typedef struct _window_progress_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    color_t color_progress;
    font_t *font;
    padding_ui8_t padding;
    uint8_t alignment;
    uint8_t height_progress;
    const char *format;
    float value;
    float min;
    float max;
} window_progress_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const window_class_progress_t window_class_progress;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WINDOW_PROGRESS_H
