// screen_qr_error.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include <array>
#include "screen.hpp"
#include "screen_reset_error.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include <gui/qr.hpp>
#include <gui/text_error_url.hpp>
#include <option/has_leds.h>
#if HAS_LEDS()
    #include "led_animations/animator.hpp"
#endif

struct ScreenErrorQR : public ScreenResetError {

    window_header_t header;
    window_text_t err_title;
    window_text_t err_description;
    window_icon_t hand_icon;
    QRErrorUrlWindow qr;
    window_text_t help_txt;
    TextErrorUrlWindow help_link;
    window_text_t qr_code_txt;
#if HAS_LEDS()
    AnimatorLCD::AnimationGuard anim;
#endif
    BasicWindow title_line;

public:
    ScreenErrorQR();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
