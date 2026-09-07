#include "screen_printer_setup.hpp"

#include <ScreenHandler.hpp>
#include <img_resources.hpp>
#include <common/nozzle_diameter.hpp>
#include <MItem_tools.hpp>
#include <option/has_heaters_selftest_revise.h>

using namespace screen_printer_setup_private;

namespace {

constexpr Rect16 prompt_rect = Rect16::fromLTWH(0, GuiDefaults::HeaderHeight, GuiDefaults::ScreenWidth, 32);
constexpr Rect16 menu_rect = Rect16::fromLTRB(0, prompt_rect.Bottom() + 4, prompt_rect.Right(), GuiDefaults::ScreenHeight);

}; // namespace

// ------------------------------------------------
// MI_DONE
// ------------------------------------------------
MI_DONE::MI_DONE()
    : IWindowMenuItem(_("Done"), &img::ok_16x16) {
}

void MI_DONE::click(IWindowMenu &) {
    config_store().printer_hw_config_done.set(true);

#if HAS_SELFTEST()
    // If the screen was open as a part of RevisePrinterStatus selftest part, goes to the next part
    // Otherwise, this is ignored
    marlin_client::FSM_response(PhasesSelftest::RevisePrinterStatus_revise, Response::Done);
    #if HAS_HEATERS_SELFTEST_REVISE()
    // Same, for the gcode-based heater selftest's revise-printer-setup phase (ignored if not active).
    marlin_client::FSM_response(PhasesHeatersSelftest::revise_revise, Response::Done);
    #endif
#endif

    Screens::Access()->Close();
}

// ------------------------------------------------
// ScreenPrinterSetup
// ------------------------------------------------
bool ScreenPrinterSetup::should_show() {
    return !config_store().printer_hw_config_done.get();
}

ScreenPrinterSetup::ScreenPrinterSetup()
    : ScreenMenu(_("PRINTER SETUP")) //
{
    ClrMenuTimeoutClose(); // don't close on menu timeout

#if HAS_INDX()
    // If INDX Nozzles were not calibrated yet, it doesn't make sense to display them.
    // They can not be enabled in this menu, so if the the item is disabled, hide it.
    if (!Item<MI_TOOLHEAD_SETTINGS>().IsEnabled()) {
        Item<MI_TOOLHEAD_SETTINGS>().hide();
    }
#endif // HAS_INDX()
}
