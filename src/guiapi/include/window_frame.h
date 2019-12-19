// window_frame.h

#ifndef _WINDOW_FRAME_H
#define _WINDOW_FRAME_H

#include <inttypes.h>

#include "guitypes.h"
#include "window.h"

#pragma pack(push)
#pragma pack(1)

typedef struct _window_class_frame_t {
    window_class_t cls;
} window_class_frame_t;

typedef struct _window_frame_t {
    window_t win;
    color_t color_back;
} window_frame_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const window_class_frame_t window_class_frame;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WINDOW_FRAME_H
