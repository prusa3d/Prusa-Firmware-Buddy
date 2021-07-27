//ScreenPrintingModel.cpp
#include "ScreenPrintingModel.hpp"
#include "config.h"
#include "i18n.h"
#include "ScreenHandler.hpp"

ScreenPrintingModel::ScreenPrintingModel(string_view_utf8 caption)
    : AddSuperWindow<IScreenPrinting>(caption)
    // clang-format off
    , btn_tune  { { this, Rect16(8 + (15 + 64) * 0, 185, 64, 64), 0, TuneAction  }, { this, Rect16(80 * 0, 196 + 48 + 8, 80, 16), is_multiline::no } }
    , btn_pause { { this, Rect16(8 + (15 + 64) * 1, 185, 64, 64), 0, PauseAction }, { this, Rect16(80 * 1, 196 + 48 + 8, 80, 16), is_multiline::no } }
    , btn_stop  { { this, Rect16(8 + (15 + 64) * 2, 185, 64, 64), 0, StopAction  }, { this, Rect16(80 * 2, 196 + 48 + 8, 80, 16), is_multiline::no } }
// clang-format on
{}

void ScreenPrintingModel::initBtnText(btn &ref_button) {
    ref_button.txt.font = resource_font(IDR_FNT_SMALL);
    ref_button.txt.SetPadding({ 0, 0, 0, 0 });
    ref_button.txt.SetAlignment(Align_t::Center());
}

void ScreenPrintingModel::setIconAndLabel(btn &ref_button, const btn_resource &res) {
    if (ref_button.ico.GetIdRes() != res.ico)
        ref_button.ico.SetIdRes(res.ico);
    // disregard comparing strings - just set the label every time
    ref_button.txt.SetText(_(res.txt));
}

void ScreenPrintingModel::initAndSetIconAndLabel(btn &ref_button, const btn_resource &res) {
    initBtnText(ref_button);
    setIconAndLabel(ref_button, res);
}
