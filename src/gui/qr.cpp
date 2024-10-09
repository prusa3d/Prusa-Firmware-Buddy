#include <gui/qr.hpp>

#include "window_qr.hpp"
#include <common/error_code_mangle.hpp>
#include <common/support_utils.h>
#include <common/str_utils.hpp>
#include <version/version.hpp>

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
    StringBuilder builder { buffer };
    builder.append_printf("https://prusa.io/%05d", map_error_code(ec));
    if (config_store().devhash_in_qr.get()) {
        {
            char printer_code[10] = {};
            printerCode(printer_code);
            builder.append_string("/");
            builder.append_string(printer_code);
        }
        {
            char version[10] = {};
            version::fill_project_version_no_dots(version, sizeof(version));
            builder.append_string("/");
            builder.append_string(version);
        }
    }
    Invalidate();
}
