#include "screen_qr_error.hpp"
#include "config.h"
#include <stdlib.h>
#include "ScreenHandler.hpp"
#include "display.h"
#include "errors.h"

screen_qr_error_data_t::screen_qr_error_data_t()
    : AddSuperWindow<screen_t>()
    , errText(this, Rect16(8, 0, 224, 25), is_multiline::no)
    , errDescription(this, Rect16(8, 30, 224, 95), is_multiline::yes)
    , info(this, Rect16(8, 275, 224, 20), is_multiline::no)
    , qr(this, Rect16(59, 140, 224, 95))
    , first_run_flag(true) {
    errText.SetBackColor(color_t::RedAlert);
    errText.font = resource_font(IDR_FNT_BIG);
    errText.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)get_actual_error()->err_title));

    errDescription.SetBackColor(color_t::RedAlert);
    errDescription.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)get_actual_error()->err_text));

    info.SetBackColor(color_t::RedAlert);
    info.SetAlignment(ALIGN_CENTER);
    static const char hlp[] = "help.prusa3d.com";
    info.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)hlp));

    qr.px_per_module = 2;
    error_url_long(qr_text.data(), qr_text.size(), 1);
    qr.text = qr_text.data();
}

void screen_qr_error_data_t::unconditionalDraw() {
    super::unconditionalDraw();
    display::FillRect(Rect16(8, 25, 224, 2), color_t::White);
}

void screen_qr_error_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if ((event == GUI_event_t::CLICK) || (event == GUI_event_t::BTN_DN)) {
        Screens::Access()->Close();
        return;
    }
    if (!first_run_flag)
        return;
    first_run_flag = false;
    //unconditionalDraw(); // todo why?
    SuperWindowEvent(sender, event, param);
}
