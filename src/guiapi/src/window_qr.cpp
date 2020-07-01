// window_qr.c
#include <algorithm>

#include "window_qr.hpp"
#include "gui.hpp"
#include "display.h"
#include "qrcodegen.h"

/// window-init call-back
/// Default setting
void window_qr_init(window_qr_t *window) {
    window->border = 4;
    window->px_per_module = 3;
    window->bg_color = COLOR_WHITE;
    window->px_color = COLOR_BLACK;
    window->align = ALIGN_CENTER;
}

/// Takes \param text and writes QR code into \param qrcode.
/// \returns true if QR code was generated.
/// Use this to allocate \param qrcode buffer:
/// uint8_t qrcode[qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version_max)];
bool generate_qr(const char *text, uint8_t qrcode[]) {
    uint8_t temp_buff[qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version_max)];
    return qrcodegen_encodeText(text, temp_buff, qrcode, qrcodegen_Ecc_LOW, 1, qr_version_max, qrcodegen_Mask_AUTO, true);
}

/// \returns number of modules of QR code (\param qrcode).
/// Does not include size of border.
/// Size is always the same for X and Y dimensions.
int get_qr_size(uint8_t *qrcode) {
    return qrcodegen_getSize(qrcode);
}

/// \returns size of QR code (\param qrcode) in pixels.
/// Includes size of borders.
/// Size is always the same for X and Y dimensions.
int get_qr_px_size(uint8_t qrcode[], const window_qr_t *const window) {
    return window->px_per_module * (2 * window->border + qrcodegen_getSize(qrcode));
}

/// Draw QR code to the screen.
///
void draw_qr(uint8_t qrcode[], const window_qr_t *const window) {

    const uint8_t msize = window->px_per_module;
    const uint16_t border = window->border;
    const uint16_t size = get_qr_size(qrcode);
    const uint16_t px_size = get_qr_px_size(qrcode, window);

    uint16_t x0 = 0;
    uint16_t y0 = 0;

    ///alignment
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

    x0 += window->rect.x + window->border * msize;
    y0 += window->rect.y + window->border * msize;

    /// FIXME paint border at once (fill_between_rect) - it's faster
    /// paint QR code
    for (int y = -border; y < (size + border); ++y)
        for (int x = -border; x < (size + border); ++x)
            display::FillRect(rect_ui16(x0 + x * msize, y0 + y * msize, msize, msize), ((qrcodegen_getModule(qrcode, x, y) ? window->px_color : window->bg_color)));
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
