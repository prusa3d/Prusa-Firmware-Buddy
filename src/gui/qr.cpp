#include <gui/qr.hpp>

#include "window_qr.hpp"
#include <common/error_code_mangle.hpp>
#include <common/support_utils.h>

QRStaticStringWindow::QRStaticStringWindow(window_t *parent, Rect16 rect, Align_t align, const char *data)
    : window_aligned_t { parent, rect }
    , data { data } {
    SetAlignment(align);
}

void QRStaticStringWindow::unconditionalDraw() {
    draw_qr({
        .data = data,
        .rect = GetRect(),
        .align = GetAlignment(),
    });
}

QRErrorUrlWindow::QRErrorUrlWindow(window_t *parent, Rect16 rect, ErrCode ec)
    : QRDynamicStringWindow { parent, rect, Align_t::Center() } {
    set_error_code(ec);
}

void QRErrorUrlWindow::set_error_code(ErrCode ec) {
    uint16_t error_code = ftrstd::to_underlying(ec);
    update_error_code(error_code);
    if (config_store().devhash_in_qr.get()) {
        error_url_long(buffer.data(), buffer.size(), error_code);
    } else {
        error_url_short(buffer.data(), buffer.size(), error_code);
    }
    Invalidate();
}
