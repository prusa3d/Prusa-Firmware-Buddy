/*
 * window_logo.h
 *
 *  Created on: 28. 8. 2019
 *      Author: mcbig
 */

#ifndef WINDOW_LOGO_H_
#define WINDOW_LOGO_H_

#include "gui.h"

#pragma pack(push)
#pragma pack(1)

typedef struct _window_class_logo_t {
    window_class_t cls;
} window_class_logo_t;

typedef struct _window_logo_t {
    window_t win;

    color_t color_back;
} window_logo_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int16_t WINDOW_CLS_LOGO;

extern const window_class_logo_t window_class_logo;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* WINDOW_LOGO_H_ */
