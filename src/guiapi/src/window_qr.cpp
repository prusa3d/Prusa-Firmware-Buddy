// window_qr.c
#include "window_qr.h"
#include "gui.h"

#include "display.h"

#include "qrcodegen.h"

/// window-init call-back
void window_qr_init(window_qr_t *window) {
    window->version = 9;
    window->ecc_level = qrcodegen_Ecc_HIGH;
    window->mode = qrcodegen_Mode_ALPHANUMERIC;
    window->border = 4;
    window->px_per_module = 3;
    window->bg_color = COLOR_WHITE;
    window->px_color = COLOR_BLACK;
}

#define BORDER (window->border)
#define MSIZE  (window->px_per_module)
#define X0     (window->win.rect.x + window->border * MSIZE)
#define Y0     (window->win.rect.y + window->border * MSIZE)

/// window-draw call-back
void window_qr_draw(window_qr_t *window) {
    uint8_t temp_buff[qrcodegen_BUFFER_LEN_FOR_VERSION(window->version)];
    uint8_t qrcode_buff[qrcodegen_BUFFER_LEN_FOR_VERSION(window->version)];
    bool qr_ok;
    int size;

    if (((window->win.flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {
        qr_ok = qrcodegen_encodeText(window->text, temp_buff, qrcode_buff, window->ecc_level, window->version, window->version, qrcodegen_Mask_AUTO, true);
        if (qr_ok) {
            size = qrcodegen_getSize(qrcode_buff);
            for (int y = -BORDER; y < (size + BORDER); y++)
                for (int x = -BORDER; x < (size + BORDER); x++)
                    display::FillRect(rect_ui16(X0 + x * MSIZE, Y0 + y * MSIZE, MSIZE, MSIZE), ((qrcodegen_getModule(qrcode_buff, x, y) ? window->px_color : window->bg_color)));
        }
        window->win.flg &= ~WINDOW_FLG_INVALID;
    }
}

/// window definition
const window_class_qr_t window_class_qr = {
    {
        WINDOW_CLS_QR,
        sizeof(window_qr_t),
        (window_init_t *)window_qr_init,
        0,
        (window_draw_t *)window_qr_draw,
        0,
    },
};
