#include <gui/text_error_url.hpp>

#include <common/error_code_mangle.hpp>
#include <common/str_utils.hpp>

TextErrorUrlWindow::TextErrorUrlWindow(window_t *parent, Rect16 rect, ErrCode ec)
    : window_text_t { parent, rect, is_multiline::no } {
    set_error_code(ec);
}

void TextErrorUrlWindow::set_error_code(ErrCode ec) {
    StringBuilder { buffer }.append_printf("prusa.io/%05u", map_error_code(ec));
    SetText(string_view_utf8::MakeRAM(buffer.data()));
    Invalidate();
}
