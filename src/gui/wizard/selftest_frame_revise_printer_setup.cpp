#include "selftest_frame_revise_printer_setup.hpp"

#include <gui/ScreenHandler.hpp>
#include <gui/screen_printer_setup.hpp>

SelftestFrameRevisePrinterSetup::SelftestFrameRevisePrinterSetup(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : SelftestFrameWithRadio(parent, ph, data)
    , text(this, {}, is_multiline::yes) //
{
    const auto parent_rect = parent->GetRect();
    const auto margin = 16;

    text.SetRect(Rect16::fromLTRB(parent_rect.Left() + margin, parent_rect.Top() + margin, parent_rect.Right() - margin, radio.GetRect().Top() - margin));
    text.SetAlignment(Align_t::Center());
    change();
}

void SelftestFrameRevisePrinterSetup::change() {
    switch (phase_current) {

    case PhasesSelftest::RevisePrinterStatus_ask_revise:
        text.SetText(_("Attention, the test has failed.\nThis could have been caused by a wrong configuration.\n\nDo you want to revise your printer configuration?"));
        break;

    case PhasesSelftest::RevisePrinterStatus_revise:
        text.SetText({});

        // The screen sends Response::Done on MI_DONE click
        if (!Screens::Access()->IsScreenOnStack<ScreenPrinterSetup>()) {
            Screens::Access()->Open<ScreenPrinterSetup>();
        }
        break;

    case PhasesSelftest::RevisePrinterStatus_ask_retry:
        text.SetText(_("Do you wish to retry the failed selftest?"));
        break;

    default:
        break;
    }
}
