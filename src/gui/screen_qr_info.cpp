#include "screen_qr_info.hpp"
#include "config.h"
#include <stdlib.h>
#include "ScreenHandler.hpp"
#include "../../gui/wizard/selftest.h"
#include "stm32f4xx_hal.h"

screen_qr_info_data_t::screen_qr_info_data_t()
    : window_frame_t(&warning)
    , warning(this, rect_ui16(8, 25, 224, 95))
    , button(this, rect_ui16(8, 280, 224, 30))
    , qr(this, rect_ui16(28, 85, 224, 95)) {
    warning.font = resource_font(IDR_FNT_TERMINAL);
    warning.SetAlignment(ALIGN_HCENTER);
    static const char slftNA[] = "selfTest-data not\n    available";
    static const char slftEx[] = "selfTest-data expired";
    static const char slftRe[] = "selfTest-data relevant";
    if (last_selftest_time == 0)
        warning.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)slftNA));
    else if ((HAL_GetTick() / 1000 - last_selftest_time) > LAST_SELFTEST_TIMEOUT)
        warning.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)slftEx));
    else
        warning.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)slftRe));

    button.font = resource_font(IDR_FNT_BIG);
    button.SetBackColor(COLOR_WHITE);
    button.SetTextColor(COLOR_BLACK);
    button.SetAlignment(ALIGN_HCENTER);
    static const char rtn[] = "RETURN";
    button.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)rtn));

    qr.ecc_level = qrcodegen_Ecc_MEDIUM;
    create_path_info_4service(qr_text.data(), qr_text.size());
    qr.text = qr_text.data();
}

int screen_qr_info_data_t::event(window_t *sender, uint8_t event, void *param) {
    if ((event == WINDOW_EVENT_CLICK) || (event == WINDOW_EVENT_BTN_DN)) {
        Screens::Access()->Close();
        return (1);
    }
    return (0);
}
