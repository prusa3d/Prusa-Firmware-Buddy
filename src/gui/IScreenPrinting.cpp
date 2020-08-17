#include "IScreenPrinting.hpp"
#include "config.h"
#include "marlin_client.h"
#include "marlin_server.h"
#include "guitypes.hpp"    //font_meas_text
#include "stm32f4xx_hal.h" //HAL_GetTick
#include "i18n.h"
#include "ScreenHandler.hpp"

IScreenPrinting::IScreenPrinting(string_view_utf8 caption)
    : window_frame_t()
    , header(this)
    , footer(this)
    // clang-format off
    , btn_tune  { { this, Rect16(8 + (15 + 64) * 0, 185, 64, 64), 0, TuneAction  }, { this, Rect16(80 * 0, 196 + 48 + 8, 80, 22) } }
    , btn_pause { { this, Rect16(8 + (15 + 64) * 1, 185, 64, 64), 0, PauseAction }, { this, Rect16(80 * 1, 196 + 48 + 8, 80, 22) } }
    , btn_stop  { { this, Rect16(8 + (15 + 64) * 2, 185, 64, 64), 0, StopAction  }, { this, Rect16(80 * 2, 196 + 48 + 8, 80, 22) } }
// clang-format on
{
    header.SetText(caption);

    initAndsetIconAndLabel(btn_tune, res_tune);
    initAndsetIconAndLabel(btn_pause, res_pause);
    initAndsetIconAndLabel(btn_stop, res_stop);

    ths = this;
}

IScreenPrinting::~IScreenPrinting() {
    ths = nullptr;
}

void IScreenPrinting::initBtnText(btn &ref_button) {
    ref_button.txt.font = resource_font(IDR_FNT_SMALL);
    ref_button.txt.SetPadding({ 0, 0, 0, 0 });
    ref_button.txt.SetAlignment(ALIGN_CENTER);
}

void IScreenPrinting::setIconAndLabel(btn &ref_button, const btn_resource &res) {
    if (ref_button.ico.GetIdRes() != res.ico)
        ref_button.ico.SetIdRes(res.ico);
    // disregard comparing strings - just set the label every time
    ref_button.txt.SetText(_(res.txt));
}

void IScreenPrinting::initAndsetIconAndLabel(btn &ref_button, const btn_resource &res) {
    initBtnText(ref_button);
    setIconAndLabel(ref_button, res);
}

/******************************************************************************/
//static methods to be pointed by fnc pointers
void IScreenPrinting::StopAction() {
    if (IScreenPrinting::ths)
        IScreenPrinting::ths->stopAction();
}
void IScreenPrinting::PauseAction() {
    if (IScreenPrinting::ths)
        IScreenPrinting::ths->pauseAction();
}
void IScreenPrinting::TuneAction() {
    if (IScreenPrinting::ths)
        IScreenPrinting::ths->tuneAction();
}

IScreenPrinting *IScreenPrinting::ths = nullptr;
bool IScreenPrinting::CanOpen() {
    return IScreenPrinting::ths == nullptr;
}
