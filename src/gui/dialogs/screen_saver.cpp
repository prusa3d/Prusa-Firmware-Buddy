/**
 * @file screen_saver.cpp
 * @author Radek Vana
 * @date 2021-03-04
 */

#include "screen_saver.hpp"
#include "GuiDefaults.hpp"

ScreenSaver::ScreenSaver()
    : AddSuperWindow<TimedDialog>(GuiDefaults::RectScreen, open_period)
    , dummy_txt(this, rect, is_multiline::yes) {
    dummy_txt.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)("SCREEN SAVER\nmove knob to close")));
}
