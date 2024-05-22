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

    static const constexpr uint16_t fw_line_top = 265;
    static const constexpr Rect16 description_rect = Rect16(13, title_rect.Bottom() + 5, display::GetW() - 26, fw_line_top - title_rect.Bottom() - 10);

    // Expected size of the description text
    static const constexpr Font description_font = Font::small;
    static const constexpr size_t description_char_width = description_rect.Width() / width(description_font);
    static const constexpr size_t description_char_height = description_rect.Height() / height(description_font);
    static const constexpr size_t description_expected_chars = description_char_width * description_char_height;

protected:
    window_text_t header; ///< BSOD, HARDFAULT, WATCHDOG, Needs to be filled in child
    window_text_t title; ///< Title with hardfault source or filename of bsod, Needs to be filled in child
    window_text_t description; ///< Description of error, Needs to be filled in child
#if HAS_LEDS()
    AnimatorLCD::AnimationGuard anim; ///< Blue flashing
#endif /*HAS_LEDS()*/
};
