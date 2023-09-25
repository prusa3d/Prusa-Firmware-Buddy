/**
 * @file selftest_frame_firstlayer.cpp
 */

#include "selftest_frame_firstlayer.hpp"
#include "i18n.h"
#include "selftest_firstlayer_type.hpp"

SelftestFrameFirstLayer::SelftestFrameFirstLayer(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrame>(parent, ph, data)
    , footer(this
#if FOOTER_LINES__ == 1
          ,
          footer::Item::nozzle, footer::Item::bed
#endif
#if defined(FOOTER_HAS_LIVE_Z)
          ,
          footer::Item::live_z
#endif
#if defined(FOOTER_HAS_SHEETS)
          ,
          footer::Item::sheets
#endif
          ,
          footer::Item::filament)
    , text(this, Rect16(WizardDefaults::MarginLeft, 40, GuiDefaults::RectScreen.Width() - WizardDefaults::MarginLeft * 2, 150), is_multiline::yes, is_closed_on_click_t::no, _(text_str))
    , progress(this, Rect16(WizardDefaults::MarginLeft, 190 + 30, GuiDefaults::RectScreen.Width() - 2 * WizardDefaults::MarginLeft, 8))
    , live_z(this, { int16_t(WizardDefaults::MarginLeft), 190 }, Width() - WizardDefaults::MarginLeft * 2) {
    CaptureNormalWindow(live_z);
    live_z.Idle();
    progress.SetColor(COLOR_LIME);
}

void SelftestFrameFirstLayer::change() {
    switch (phase_current) {
    case PhasesSelftest::FirstLayer_mbl:
        live_z.Idle();
        break;
    case PhasesSelftest::FirstLayer_print:
        live_z.Activate();
        break;
    default:
        break;
    }
};
