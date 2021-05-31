// window_qr.cpp
#include <algorithm>
#include <math.h>

#include "window_qr.hpp"
#include "gui.hpp"
#include "display.h"
#include "qrcodegen.h"

/// QR API
/// TODO create separate class somewhere else

/// private
bool generate_qr_nobuff(const char *text, uint8_t qrcode[]) {
    uint8_t buffer[qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version_max)];
    return generate_qr(text, qrcode, buffer);
}

/// public
bool generate_qr(const char *text, uint8_t qrcode[]) {
    return generate_qr_nobuff(text, qrcode);
}

/// public
bool generate_qr(const char *text, uint8_t qrcode[], uint8_t buffer[]) {
    if (buffer == nullptr)
        return generate_qr_nobuff(text, qrcode);

    return qrcodegen_encodeText(text, buffer, qrcode, qrcodegen_Ecc_LOW, 1, qr_version_max, qrcodegen_Mask_AUTO, true);
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
        ppm = std::max(1, (int)floor(std::min(uint16_t(window->rect.Height()), uint16_t(window->rect.Width())) / float(size_w_bord)));
    }
    const uint16_t px_size = get_qr_px_size(qrcode, window, ppm);

    /// alignment
    if (window->align.Horizontal() == Align_t::horizontal::center) {
        x0 = std::max(0, (window->rect.Width() - px_size)) / 2;
    } else if (window->align.Horizontal() == Align_t::horizontal::right) {
        x0 = std::max(0, (window->rect.Width() - px_size));
    }

    if (window->align.Vertical() == Align_t::vertical::center) {
        y0 = std::max(0, (window->rect.Height() - px_size)) / 2;
    } else if (window->align.Vertical() == Align_t::vertical::bottom) {
        y0 = std::max(0, (window->rect.Height() - px_size));
    }

    /// move to window location
    x0 += window->rect.Left() + window->border * ppm;
    y0 += window->rect.Top() + window->border * ppm;

    /// FIXME paint border at once (fill_between_rect) - it's faster
    /// paint QR code
    for (int y = -border; y < (size + border); ++y)
        for (int x = -border; x < (size + border); ++x)
            display::FillRect(Rect16(x0 + x * ppm, y0 + y * ppm, ppm, ppm), ((qrcodegen_getModule(qrcode, x, y) ? window->px_color : window->bg_color)));
}

/// QR Window

window_qr_t::window_qr_t(window_t *parent, Rect16 rect)
    : window_t(parent, rect)
// , version(9)
// , ecc_level(qrcodegen_Ecc_HIGH)
// , mode(qrcodegen_Mode_ALPHANUMERIC)
// , border(4)
// , px_per_module(3)
// , bg_color(COLOR_WHITE)
//    , px_color(COLOR_BLACK)
{
}

/// window definition
// const window_class_qr_t window_class_qr = {
//     {
//         WINDOW_CLS_QR,
//         sizeof(window_qr_t),
//         (window_init_t *)window_qr_init,
//         0,
//         (window_draw_t *)window_qr_draw,
//         0,
//     },
// };
