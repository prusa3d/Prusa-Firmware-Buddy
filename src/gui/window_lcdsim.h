// window_lcdsim.h

#ifndef _WINDOW_LCDSIM_H
#define _WINDOW_LCDSIM_H

#include "window.h"

#pragma pack(push)
#pragma pack(1)

typedef struct _window_class_lcdsim_t {
    window_class_t cls;
} window_class_lcdsim_t;

typedef struct _window_lcdsim_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    font_t *font;
} window_lcdsim_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int16_t WINDOW_CLS_LCDSIM;

//extern int16_t window_lcdsim_create(int16_t id_parent, rect_ui16_t rc, font_t* font, color_t color_back, color_t color_text);

extern const window_class_lcdsim_t window_class_lcdsim;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WINDOW_LCDSIM_H
