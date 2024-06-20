#include "screen_printer_setup.hpp"

#include <ScreenHandler.hpp>
#include <img_resources.hpp>
#include <str_utils.hpp>
#include <common/nozzle_diameter.hpp>
#include <MItem_tools.hpp>

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
    config_store().printer_setup_done.set(true);

    // If the screen was open as a part of RevisePrinterStatus selftest part, goes to the next part
    // Otherwise, this is ignored
    marlin_client::FSM_response(PhasesSelftest::RevisePrinterStatus_revise, Response::Done);

    Screens::Access()->Close();
}

// ------------------------------------------------
// MI_NOZZLE_DIAMETER_HELP
// ------------------------------------------------
MI_NOZZLE_DIAMETER_HELP::MI_NOZZLE_DIAMETER_HELP()
    : IWindowMenuItem(_("What nozzle diameter do I have?"), &img::question_16x16) {
}

void MI_NOZZLE_DIAMETER_HELP::click(IWindowMenu &) {
    MsgBoxInfo(_("You can determine the nozzle diameter by counting the markings (dots) on the nozzle:\n"
                 "  0.40 mm nozzle: 3 dots\n"
                 "  0.60 mm nozzle: 4 dots\n\n"
                 "For more information, visit prusa.io/nozzle-types"),
        Responses_Ok);
}

// ------------------------------------------------
// MI_NOZZLE_DIAMETER_T
// ------------------------------------------------
#if HAS_TOOLCHANGER()
MI_NOZZLE_DIAMETER_T::MI_NOZZLE_DIAMETER_T(uint8_t tool_index)
    : WiSpin(config_store().get_nozzle_diameter(tool_index), nozzle_diameter_spin_config, _(tool_name(tool_index)))
    , tool_index(tool_index) {
    set_is_hidden(!prusa_toolchanger.is_tool_enabled(tool_index));
}

void MI_NOZZLE_DIAMETER_T::OnClick() {
    config_store().set_nozzle_diameter(tool_index, value());
}
#endif

// ------------------------------------------------
// MI_NOZZLE_DIAMETER_MENU
// ------------------------------------------------
#if HAS_TOOLCHANGER()
MI_NOZZLE_DIAMETER_MENU::MI_NOZZLE_DIAMETER_MENU()
    : IWindowMenuItem(_("Nozzle Diameter"), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_NOZZLE_DIAMETER_MENU::click(IWindowMenu &) {
    Screens::Access()->Open<ScreenNozzleDiameter>();
}
#endif

// ------------------------------------------------
// ScreenNozzleDiameter
// ------------------------------------------------
#if HAS_TOOLCHANGER()
ScreenNozzleDiameter::ScreenNozzleDiameter()
    : ScreenPropertySpecific(_("NOZZLE DIAMETER")) {}
#endif

// ------------------------------------------------
// ScreenPrinterSetup
// ------------------------------------------------
ScreenPrinterSetup::ScreenPrinterSetup()
    : ScreenMenu(_("PRINTER SETUP")) //
{
    ClrMenuTimeoutClose(); // don't close on menu timeout

#if HAS_TOOLCHANGER()
    const bool show_tool_specific_menus = prusa_toolchanger.is_toolchanger_enabled();
    Item<MI_NOZZLE_DIAMETER_HELP>().set_is_hidden(show_tool_specific_menus);
    Item<MI_NOZZLE_DIAMETER>().set_is_hidden(show_tool_specific_menus);
    Item<MI_NOZZLE_DIAMETER_MENU>().set_is_hidden(!show_tool_specific_menus);
#endif
}
