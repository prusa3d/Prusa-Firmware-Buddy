//ScreenPrintingModel.cpp
#include "ScreenPrintingModel.hpp"
#include "config.h"
#include "i18n.h"
#include "ScreenHandler.hpp"

ScreenPrintingModel::ScreenPrintingModel(string_view_utf8 caption)
    : AddSuperWindow<IScreenPrinting>(caption)
// clang-format off
#if defined(USE_ST7789)
    , btn_tune  { { this, Rect16(8 + (15 + 64) * 0, 185, 64, 64), nullptr, TuneAction  }, { this, Rect16(80 * 0, 196 + 48 + 8, 80, 16), is_multiline::no } }
    , btn_pause { { this, Rect16(8 + (15 + 64) * 1, 185, 64, 64), nullptr, PauseAction }, { this, Rect16(80 * 1, 196 + 48 + 8, 80, 16), is_multiline::no } }
    , btn_stop  { { this, Rect16(8 + (15 + 64) * 2, 185, 64, 64), nullptr, StopAction  }, { this, Rect16(80 * 2, 196 + 48 + 8, 80, 16), is_multiline::no } }
#elif defined(USE_ILI9488)
    , btn_tune  { { this, Rect16(114, 185, 64, 64), nullptr, TuneAction  }, { this, Rect16(114, 185 + 64 + 4, 64, 17), is_multiline::no } }
    , btn_pause { { this, Rect16(114 + (64 + 30), 185, 64, 64), nullptr, PauseAction }, { this, Rect16(114 + (64 + 15), 185 + 64 + 4, 94, 17), is_multiline::no } }
    , btn_stop  { { this, Rect16(114 + (64 + 30) * 2, 185, 64, 64), nullptr, StopAction  }, { this, Rect16(114 + (64 + 30) * 2, 185 + 64 + 4, 64, 17), is_multiline::no } }
#endif // USE_<display>
// clang-format on
{}

void ScreenPrintingModel::initBtnText(btn &ref_button) {
    ref_button.txt.font = resource_font(IDR_FNT_SMALL);
    ref_button.txt.SetPadding({ 0, 0, 0, 0 });
    ref_button.txt.SetAlignment(Align_t::Center());
}

void ScreenPrintingModel::setIconAndLabel(btn &ref_button, const BtnResource &res) {
    ref_button.ico.SetRes(res.second);
    // disregard comparing strings - just set the label every time
    ref_button.txt.SetText(_(res.first));
}

void ScreenPrintingModel::initAndSetIconAndLabel(btn &ref_button, const BtnResource &res) {
    initBtnText(ref_button);
    setIconAndLabel(ref_button, res);
}
