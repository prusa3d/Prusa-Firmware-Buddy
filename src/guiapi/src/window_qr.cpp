// window_qr.c
#include <algorithm>
#include <math.h>

#include "window_qr.hpp"
#include "gui.hpp"
#include "display.h"
#include "qrcodegen.h"

/// QR API

bool generate_qr(const char *text, uint8_t qrcode[]) {
    uint8_t temp_buff[qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version_max)];
    return qrcodegen_encodeText(text, temp_buff, qrcode, qrcodegen_Ecc_LOW, 1, qr_version_max, qrcodegen_Mask_AUTO, true);
}

int get_qr_size(uint8_t *qrcode) {
    return qrcodegen_getSize(qrcode);
}

int get_qr_px_size(uint8_t qrcode[], const window_qr_t *const window) {
    return window->px_per_module * (2 * window->border + qrcodegen_getSize(qrcode));
}

int get_qr_px_size(uint8_t qrcode[], const window_qr_t *const window, uint16_t px_per_module) {
    return px_per_module * (2 * window->border + qrcodegen_getSize(qrcode));
}

/// Draw QR code to the screen.
///
void draw_qr(uint8_t qrcode[], const window_qr_t *const window) {

    uint8_t ppm = window->px_per_module; /// pixels per module
    const uint16_t border = window->border;
    const uint16_t size = get_qr_size(qrcode);

    uint16_t x0 = 0;
    uint16_t y0 = 0;

    /// scale QR code
    if (window->scale) {
        const uint16_t size_w_bord = size + 2 * window->border;
        ppm = std::max(1, (int)floor(std::min(window->rect.h, window->rect.w) / float(size_w_bord)));
    }
    const uint16_t px_size = get_qr_px_size(qrcode, window, ppm);

    /// alignment
    if (window->align & ALIGN_HCENTER) {
        x0 = std::max(0, (window->rect.w - px_size)) / 2;
    } else if (window->align & ALIGN_RIGHT) {
        x0 = std::max(0, (window->rect.w - px_size));
    }

    if (window->align & ALIGN_VCENTER) {
        y0 = std::max(0, (window->rect.h - px_size)) / 2;
    } else if (window->align & ALIGN_BOTTOM) {
        y0 = std::max(0, (window->rect.h - px_size));
    }

    /// move to window location
    x0 += window->rect.x + window->border * ppm;
    y0 += window->rect.y + window->border * ppm;

    /// FIXME paint border at once (fill_between_rect) - it's faster
    /// paint QR code
    for (int y = -border; y < (size + border); ++y)
        for (int x = -border; x < (size + border); ++x)
            display::FillRect(rect_ui16(x0 + x * ppm, y0 + y * ppm, ppm, ppm), ((qrcodegen_getModule(qrcode, x, y) ? window->px_color : window->bg_color)));
}

/// QR Window

/// window-init call-back
/// Default setting
void window_qr_init(window_qr_t *window) {
}

/// window-draw call-back
void window_qr_draw(window_qr_t *window) {
    if ((window->flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) != (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))
        return;

    window->flg &= ~WINDOW_FLG_INVALID;

    uint8_t qrcode[qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version_max)];
    if (!generate_qr(window->text, qrcode))
        return;

    draw_qr(qrcode, window);
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
