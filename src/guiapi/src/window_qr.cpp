// window_qr.cpp
#include "window_qr.hpp"
#include "gui.hpp"

#include "display.h"

#include "qrcodegen.h"

#define BORDER (window->border)
#define MSIZE  (window->px_per_module)
#define X0     (window->rect.x + window->border * MSIZE)
#define Y0     (window->rect.y + window->border * MSIZE)

/// window-draw call-back
void window_qr_draw(window_qr_t *window) {
    uint8_t temp_buff[qrcodegen_BUFFER_LEN_FOR_VERSION(window->version)];
    uint8_t qrcode_buff[qrcodegen_BUFFER_LEN_FOR_VERSION(window->version)];
    bool qr_ok;
    int size;

    if (window->IsEnabled() && window->IsVisible()) {
        qr_ok = qrcodegen_encodeText(window->text, temp_buff, qrcode_buff, window->ecc_level, window->version, window->version, qrcodegen_Mask_AUTO, true);
        if (qr_ok) {
            size = qrcodegen_getSize(qrcode_buff);
            for (int y = -BORDER; y < (size + BORDER); y++)
                for (int x = -BORDER; x < (size + BORDER); x++)
                    display::FillRect(rect_ui16(X0 + x * MSIZE, Y0 + y * MSIZE, MSIZE, MSIZE), ((qrcodegen_getModule(qrcode_buff, x, y) ? window->px_color : window->bg_color)));
        }
        window->Validate();
        ;
    }
}

window_qr_t::window_qr_t(window_t *parent, window_t *prev, rect_ui16_t rect)
    : window_t(parent, prev)
    , version(9)
    , ecc_level(qrcodegen_Ecc_HIGH)
    , mode(qrcodegen_Mode_ALPHANUMERIC)
    , border(4)
    , px_per_module(3)
    , bg_color(COLOR_WHITE)
    , px_color(COLOR_BLACK) {
}
