#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include <array>
#include "screen.hpp"
#include "screen_reset_error.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "led_animations/animator.hpp"

class ScreenFatalWarning : public AddSuperWindow<ScreenResetError> {

    window_header_t header;
    window_text_t err_title;
    window_text_t err_description;
    window_icon_t hand_icon;
    window_qr_t qr;
    window_text_t help_txt;
    window_text_t help_link;
    window_text_t qr_code_txt;
    AnimatorLCD::AnimationGuard anim;

public:
    ScreenFatalWarning();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
