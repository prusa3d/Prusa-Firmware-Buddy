
#include "screen_blue_error.hpp"
#include <ScreenHandler.hpp>
#include <sound.hpp>
#include <sys.h>
#include <support_utils.h>
#include <version.h>
#include <crash_dump/dump.hpp>

using namespace crash_dump;

static const constexpr Rect16 fw_version_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, ScreenBlueError::fw_line_top, GuiDefaults::ScreenWidth - 30, 20) : Rect16(6, 295, GuiDefaults::ScreenWidth - 6, 13);
static const constexpr Rect16 header_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(14, 10, 240, GuiDefaults::HeaderHeight - 10) : GuiDefaults::RectHeader;
static const constexpr Rect16 title_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 44, GuiDefaults::ScreenWidth - 60, 20) : Rect16(13, 12, GuiDefaults::ScreenWidth - 26, 20);

ScreenBlueError::ScreenBlueError()
    : ScreenResetError(fw_version_rect)
    ///@note No translations on blue screens.
    , header(this, header_rect, is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>("UNKNOWN ERROR")))
    , title(this, title_rect, is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>("Unable to show details")))
    , description(this, description_rect, is_multiline::yes)
#if HAS_LEDS()
    , anim(Animator_LCD_leds().start_animations(Fading(leds::ColorRGBW(0, 0, 255), 500), 10))
#endif /*HAS_LEDS()*/
{
    SetBlueLayout();

    // Simple text instead of header
    header.SetAlignment(Align_t::LeftTop());
    if constexpr (GuiDefaults::EnableDialogBigLayout) {
        header.set_font(Font::special);
    } else {
        header.set_font(Font::small);
    }

    description.set_font(description_font);
}
