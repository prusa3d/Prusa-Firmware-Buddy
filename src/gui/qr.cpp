#include <gui/qr.hpp>

#include "window_qr.hpp"
#include <common/error_code_mangle.hpp>
#include <common/support_utils.h>
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
    : QRErrorUrlWindow(parent, rect) {
    set_error_code(ec);
}

QRErrorUrlWindow::QRErrorUrlWindow(window_t *parent, Rect16 rect)
    : QRDynamicStringWindow { parent, rect, Align_t::Center() } {
}

void QRErrorUrlWindow::set_error_code(ErrCode ec) {
    StringBuilder builder { buffer };
    builder.append_printf("https://prusa.io/%05d", map_error_code(ec));
    if (config_store().devhash_in_qr.get()) {
        {
            std::array<char, 8> printer_code;
            printerHash(printer_code.data(), printer_code.size());
            builder.append_string("?ref_data=");
            builder.append_std_string_view({ printer_code.data(), printer_code.size() });
        }
        {
            char version[10] = {};
            version::fill_project_version_no_dots(version, sizeof(version));
            builder.append_string("-");
            builder.append_string(version);
        }
    }
    Invalidate();
}
