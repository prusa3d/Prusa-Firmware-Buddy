#include "screen_printer_setup.hpp"

#include <ScreenHandler.hpp>
#include <img_resources.hpp>

namespace screen_printer_setup_private {

constexpr Rect16 prompt_rect = Rect16::fromLTWH(0, GuiDefaults::HeaderHeight, GuiDefaults::ScreenWidth, 32);
constexpr Rect16 menu_rect = Rect16::fromLTRB(0, prompt_rect.Bottom() + 4, prompt_rect.Right(), GuiDefaults::ScreenHeight);

}; // namespace screen_printer_setup_private
using namespace screen_printer_setup_private;

MI_DONE::MI_DONE()
    : IWindowMenuItem(_("Done"), &img::ok_16x16) {
}

void MI_DONE::click(IWindowMenu &) {
    // TODO mark HW configured?
    Screens::Access()->Close();
}

ScreenPrinterSetup::ScreenPrinterSetup()
    : header(this, _("PRINTER SETUP"))
    , prompt(this, prompt_rect, is_multiline::no, is_closed_on_click_t::no, _("Confirm your printer setup"))
    , menu(this, menu_rect) //
{
    ClrMenuTimeoutClose(); // don't close on menu timeout

    prompt.SetAlignment(Align_t::Center());
    prompt.SetTextColor(COLOR_BLACK);
    prompt.SetBackColor(COLOR_WHITE);

    menu.menu.BindContainer(menu_container);
    CaptureNormalWindow(menu);
}
