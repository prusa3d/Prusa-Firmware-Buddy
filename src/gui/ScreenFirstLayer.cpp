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
    , text(this, Rect16(WizardDefaults::MarginLeft, 40, GuiDefaults::RectScreen.Width() - WizardDefaults::MarginLeft * 2, 150), is_multiline::yes, is_closed_on_click_t::no, _(text_str))
    , progress(this, { WizardDefaults::MarginLeft, 190 + 30 }, HasNumber_t::no)
    , live_z(this, { int16_t(WizardDefaults::MarginLeft), 190 }, rect.Width() - WizardDefaults::MarginLeft * 2) {
    CaptureNormalWindow(live_z);
    live_z.Idle();
}

void ScreenFirstLayer::notifyMarlinStart() { live_z.Activate(); };

void ScreenFirstLayer::stopAction() {}
void ScreenFirstLayer::pauseAction() {}
void ScreenFirstLayer::tuneAction() {}
