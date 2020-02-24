// window_qr.h

#ifndef _WINDOW_QR_H
#define _WINDOW_QR_H

#include "window.h"

#include "qrcodegen.h"

#pragma pack(push)
#pragma pack(1)

typedef struct {
    window_class_t cls;
} window_class_qr_t;

typedef struct {
    window_t win;
    char *text;
    int version;
    enum qrcodegen_Ecc ecc_level;
    enum qrcodegen_Mode mode;
    uint8_t border;
    uint8_t px_per_module;
    color_t bg_color;
    color_t px_color;
} window_qr_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const window_class_qr_t window_class_qr;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WINDOW_QR_H
