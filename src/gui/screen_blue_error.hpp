#pragma once

#include <gui.hpp>
#include <window_text.hpp>
#include <array>
#include <screen.hpp>
#include <screen_reset_error.hpp>
#include <window_header.hpp>
#include <status_footer.hpp>
#include <option/has_leds.h>
#if HAS_LEDS()
    #include <led_animations/animator.hpp>
#endif /*HAS_LEDS()*/

/**
 * @brief Blue screen with error message.
 *
 * @note No windowEvent() here, user must press reset to get rid of bluescreen.
 */
class ScreenBlueError : public AddSuperWindow<ScreenResetError> {
public:
    ScreenBlueError();

    static const constexpr ResourceId header_font = IDR_FNT_SMALL;
    static const constexpr Rect16 header_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(14, 10, 240, GuiDefaults::HeaderHeight - 10) : GuiDefaults::RectHeader;
    static const constexpr Rect16 title_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 44, display::GetW() - 60, 20) : Rect16(13, 12, display::GetW() - 26, 20);
    static const constexpr Rect16 description_rect = Rect16(13, title_rect.Bottom() + 5, display::GetW() - 26, fw_version_rect.Top() - title_rect.Bottom() - 10);

    // Expected size of the description text
    static const constexpr ResourceId description_font = IDR_FNT_SMALL;
    static const constexpr size_t description_char_width = description_rect.Width() / resource_font_size(description_font).w;
    static const constexpr size_t description_char_height = description_rect.Height() / resource_font_size(description_font).h;
    static const constexpr size_t description_expected_chars = description_char_width * description_char_height;

protected:
    window_text_t header; ///< BSOD, HARDFAULT, WATCHDOG, Needs to be filled in child
    window_text_t title; ///< Title with hardfault source or filename of bsod, Needs to be filled in child
    window_text_t description; ///< Description of error, Needs to be filled in child
#if HAS_LEDS()
    AnimatorLCD::AnimationGuard anim; ///< Blue flashing
#endif /*HAS_LEDS()*/
};
