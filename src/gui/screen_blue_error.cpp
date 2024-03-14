
#include "screen_blue_error.hpp"
#include <ScreenHandler.hpp>
#include <display.h>
#include <sound.hpp>
#include <sys.h>
#include <support_utils.h>
#include <version.h>
#include <crash_dump/dump.hpp>

using namespace crash_dump;

ScreenBlueError::ScreenBlueError()
    : AddSuperWindow<ScreenResetError>()
    ///@note No translations on blue screens.
    , header(this, header_rect, is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>("UNKNOWN ERROR")))
    , title(this, title_rect, is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>("Unable to show details")))
    , description(this, description_rect, is_multiline::yes)
#if HAS_LEDS()
    , anim(Animator_LCD_leds().start_animations(Fading(leds::Color(0, 0, 255), 500), 10))
#endif /*HAS_LEDS()*/
{
    SetBlueLayout();

    // Simple text instead of header
    header.SetAlignment(Align_t::LeftTop());
    if constexpr (GuiDefaults::EnableDialogBigLayout) {
        header.set_font(resource_font(IDR_FNT_SPECIAL));
    } else {
        header.set_font(resource_font(IDR_FNT_SMALL));
    }

    description.set_font(resource_font(description_font));
}
