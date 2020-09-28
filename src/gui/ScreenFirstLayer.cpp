// ScreenFirstLayer.cpp
#include "ScreenFirstLayer.hpp"
#include "config.h"
#include "marlin_client.h"
#include "marlin_server.h"
#include "guitypes.hpp"    //font_meas_text
#include "stm32f4xx_hal.h" //HAL_GetTick
#include "i18n.h"
#include "ScreenHandler.hpp"

ScreenFirstLayer::ScreenFirstLayer()
    : IScreenPrinting(_(caption))
    , text(this, Rect16(WIZARD_MARGIN_LEFT, 40, GuiDefaults::RectScreen.Width() - WIZARD_MARGIN_LEFT, 150), is_multiline::yes, is_closed_on_click_t::no, _(text_str))
    , progress(this, { WIZARD_MARGIN_LEFT, 190 + 30 })
    , live_z(this, { 100, 190 }) {
    live_z.SetCapture();
}

void ScreenFirstLayer::stopAction() {}
void ScreenFirstLayer::pauseAction() {}
void ScreenFirstLayer::tuneAction() {}
