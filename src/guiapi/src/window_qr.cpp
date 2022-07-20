// window_qr.cpp
#include <algorithm>
#include <math.h>

#include "window_qr.hpp"
#include "gui.hpp"
#include "display.h"
#include "qrcodegen.h"
#include "scratch_buffer.hpp"
#include "support_utils.h"

/// QR Window
window_qr_t::window_qr_t(window_t *parent, Rect16 rect, uint16_t err_num)
    : window_qr_t(parent, rect) {
    SetQRHeader(err_num);
}

window_qr_t::window_qr_t(window_t *parent, Rect16 rect)
    : AddSuperWindow<window_t>(parent, rect)
    // , version(9)
    // , ecc_level(qrcodegen_Ecc_HIGH)
    // , mode(qrcodegen_Mode_ALPHANUMERIC)
    , border(4)
    , px_per_module(2)
    , align(Align_t::Center())
    , scale(true) {
}

window_qr_t::window_qr_t(window_t *parent, Rect16 rect, const char *txt)
    : window_qr_t(parent, rect) {
    strncpy(text, txt, sizeof(text));
}

void window_qr_t::SetQRHeader(uint16_t err_num) {
    bool devhash_in_qr = eeprom_get_bool(EEVAR_DEVHASH_IN_QR);
    if (devhash_in_qr) {
        error_url_long(text, sizeof(text), err_num);
    } else {
        error_url_short(text, sizeof(text), err_num);
    }
    Invalidate();
}

void window_qr_t::unconditionalDraw() {
    buddy::scratch_buffer::Ownership scratch_buffer_ownership;
    scratch_buffer_ownership.acquire(/*wait=*/true);
    uint8_t *qrcode = scratch_buffer_ownership.get().buffer;
    uint8_t *qr_buff = qrcode + qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version_max);

    if (!qrcodegen_encodeText(text, qr_buff, qrcode, qrcodegen_Ecc_LOW, 1, qr_version_max, qrcodegen_Mask_AUTO, true))
        return;

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
    for (int y = -border; y < (size + border); ++y)
        for (int x = -border; x < (size + border); ++x)
            display::FillRect(Rect16(x0 + x * ppm, y0 + y * ppm, ppm, ppm), ((qrcodegen_getModule(qrcode, x, y) ? COLOR_BLACK : COLOR_WHITE)));
}
