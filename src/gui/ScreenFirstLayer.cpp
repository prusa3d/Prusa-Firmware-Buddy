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
    , text(this, Rect16(WIZARD_MARGIN_LEFT, 40, GuiDefaults::RectScreen.Width() - WIZARD_MARGIN_LEFT * 2, 150), is_multiline::yes, is_closed_on_click_t::no, _(text_str))
    , progress(this, { WIZARD_MARGIN_LEFT, 190 + 30 }, HasNumber_t::no)
    , live_z(this, { int16_t(WIZARD_MARGIN_LEFT), 190 }, rect.Width() - WIZARD_MARGIN_LEFT * 2) {
    CaptureNormalWindow(live_z);
    live_z.Idle();
}

void ScreenFirstLayer::notifyMarlinStart() { live_z.Activate(); };

void ScreenFirstLayer::stopAction() {}
void ScreenFirstLayer::pauseAction() {}
void ScreenFirstLayer::tuneAction() {}
