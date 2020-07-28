#include "IScreenPrinting.hpp"
#include "config.h"
#include "marlin_client.h"
#include "marlin_server.h"
#include "guitypes.h"      //font_meas_text
#include "stm32f4xx_hal.h" //HAL_GetTick
#include "../lang/i18n.h"
#include "ScreenHandler.hpp"

IScreenPrinting::IScreenPrinting(string_view_utf8 caption)
    : window_frame_t()
    , header(this)
    , footer(this)
    // clang-format off
    , btn_tune  { { this, rect_ui16(8 + (15 + 64) * 0, 185, 64, 64), 0, []() { /*screen_open(get_scr_menu_tune()->id);*/    } }, { this, rect_ui16(80 * 0, 196 + 48 + 8, 80, 22) } }
    , btn_pause { { this, rect_ui16(8 + (15 + 64) * 1, 185, 64, 64), 0, []() { marlin_gcode("M118 A1 action:pause");        } }, { this, rect_ui16(80 * 1, 196 + 48 + 8, 80, 22) } }
    , btn_stop  { { this, rect_ui16(8 + (15 + 64) * 2, 185, 64, 64), 0, []() {                                              } }, { this, rect_ui16(80 * 2, 196 + 48 + 8, 80, 22) } }
// clang-format on
{
    header.SetText(caption);

    initAndsetIconAndLabel(btn_tune, res_tune);
    initAndsetIconAndLabel(btn_pause, res_pause);
    initAndsetIconAndLabel(btn_stop, res_stop);
}
/*
void IScreenPrinting::enableButton(btn &ref_button) {
    if (ref_button.ico.IsFocused()) {// move to pause when tune is focused
        btn_pause.ico.SetFocus();
    }

    ref_button.ico.SwapBW();
    ref_button.ico.Disable(); // can't be focused
    ref_button.ico.Invalidate();
}

void IScreenPrinting::disableButton(btn &ref_button) {
    ref_button.ico.UnswapBW();
    ref_button.ico.Enable(); // can be focused
    ref_button.ico.Invalidate();
}
*/
void IScreenPrinting::initBtnText(btn &ref_button) {
    ref_button.txt.font = resource_font(IDR_FNT_SMALL);
    ref_button.txt.SetPadding(padding_ui8(0, 0, 0, 0));
    ref_button.txt.SetAlignment(ALIGN_CENTER);
}

void IScreenPrinting::setIconAndLabel(btn &ref_button, btn_resource res) {
    if (ref_button.ico.GetIdRes() != res.ico)
        ref_button.ico.SetIdRes(res.ico);
    // disregard comparing strings - just set the label every time
    ref_button.txt.SetText(_(res.txt));
}

void IScreenPrinting::initAndsetIconAndLabel(btn &ref_button, btn_resource res) {
    initBtnText(ref_button);
    setIconAndLabel(ref_button, res);
}
