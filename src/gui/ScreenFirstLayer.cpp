// ScreenFirstLayer.cpp
#include "ScreenFirstLayer.hpp"
#include "config.h"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "marlin_client.h"

ScreenFirstLayer::ScreenFirstLayer()
    : IScreenPrinting(_(caption))
    , text(this, Rect16(WizardDefaults::MarginLeft, 40, GuiDefaults::RectScreen.Width() - WizardDefaults::MarginLeft * 2, 150), is_multiline::yes, is_closed_on_click_t::no, _(text_str))
    , progress(this, Rect16(WizardDefaults::MarginLeft, 190 + 30, GuiDefaults::RectScreen.Width() - 2 * WizardDefaults::MarginLeft, 8))
    , live_z(this, { int16_t(WizardDefaults::MarginLeft), 190 }, Width() - WizardDefaults::MarginLeft * 2) {
    CaptureNormalWindow(live_z);
    live_z.Idle();
    progress.SetColor(COLOR_LIME);
}

void ScreenFirstLayer::notifyMarlinStart() { live_z.Activate(); };

void ScreenFirstLayer::stopAction() {}
void ScreenFirstLayer::pauseAction() {}
void ScreenFirstLayer::tuneAction() {}
void ScreenFirstLayer::windowEvent(EventLock lock, window_t *sender, GUI_event_t event, void *param) {

    if (marlin_error(MARLIN_ERR_ProbingFailed)) {
        marlin_clear_qcode_queue();
        Screens::Access()->Close();
    }
}
