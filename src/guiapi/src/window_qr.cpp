// window_qr.cpp
#include <algorithm>
#include <math.h>

#include <config_store/store_instance.hpp>
#include "window_qr.hpp"
#include "gui.hpp"
#include "display.h"
#include "qrcodegen.h"
#include "support_utils.h"

/// QR Window
window_qr_t::window_qr_t(window_t *parent, Rect16 rect, uint16_t err_num, Align_t align)
    : window_qr_t(parent, rect, align) {
    SetQRHeader(err_num);
}

window_qr_t::window_qr_t(window_t *parent, Rect16 rect, Align_t align)
    : AddSuperWindow<window_t>(parent, rect)
    , error_num(0)
    , border(2)
    , px_per_module(2)
    , align(align)
    , scale(true) {
    text[0] = '\0';
}

window_qr_t::window_qr_t(window_t *parent, Rect16 rect, const char *txt)
    : window_qr_t(parent, rect, Align_t::Center()) {
    strncpy(text, txt, sizeof(text));
}

void window_qr_t::SetQRHeader(uint16_t err_num) {
    error_num = err_num;
    bool devhash_in_qr = config_store().devhash_in_qr.get();
    if (devhash_in_qr) {
        error_url_long(text, sizeof(text), err_num);
    } else {
        error_url_short(text, sizeof(text), err_num);
    }
    Invalidate();
}

void window_qr_t::SetText(const char *txt) {
    strlcpy(text, txt, sizeof(text));
    Invalidate();
}

const char *window_qr_t::GetQRLongText() {
    error_url_long(text, sizeof(text), error_num);
    return text;
}

const char *window_qr_t::GetQRShortText() {
    error_url_short(text, sizeof(text), error_num);
    return text;
}

void window_qr_t::unconditionalDraw() {
    /// Drawn QR code, 353 B on stack
    uint8_t qrcode[qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version_max)];

    { // Create QR code
        /// Temporary buffer, 353 B using display buffer
        display::BorrowBuffer buffer;
        assert(display::BufferPixelSize() >= qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version_max));
        if (!qrcodegen_encodeText(text, buffer, qrcode, qrcodegen_Ecc_LOW, 1, qr_version_max, qrcodegen_Mask_AUTO, true)) {
            return;
        }
    }

    uint8_t ppm = px_per_module; /// pixels per module
    const uint16_t size = qrcodegen_getSize(qrcode);

    uint16_t x0 = 0;
    uint16_t y0 = 0;

    /// scale QR code
    if (scale) {
        const uint16_t size_w_bord = size + 2 * border;
        ppm = std::max(1, (int)floor(std::min(uint16_t(Height()), uint16_t(Width())) / float(size_w_bord)));
    }
    const uint16_t px_size = ppm * (2 * border + qrcodegen_getSize(qrcode));

    /// alignment
    if (align.Horizontal() == Align_t::horizontal::center) {
        x0 = std::max(0, (Width() - px_size)) / 2;
    } else if (align.Horizontal() == Align_t::horizontal::right) {
        x0 = std::max(0, (Width() - px_size));
    }

    if (align.Vertical() == Align_t::vertical::center) {
        y0 = std::max(0, (Height() - px_size)) / 2;
    } else if (align.Vertical() == Align_t::vertical::bottom) {
        y0 = std::max(0, (Height() - px_size));
    }

    /// move to window location
    x0 += Left() + border * ppm;
    y0 += Top() + border * ppm;

    /// FIXME paint border at once (fill_between_rect) - it's faster
    /// paint QR code
    for (int y = -border; y < (size + border); ++y) {
        for (int x = -border; x < (size + border); ++x) {
            display::FillRect(Rect16(x0 + x * ppm, y0 + y * ppm, ppm, ppm), ((qrcodegen_getModule(qrcode, x, y) ? COLOR_BLACK : COLOR_WHITE)));
        }
    }
}
