/**
 * @file screen_help_fw_update.hpp
 */
#pragma once
#include "gui.hpp"
#include "screen.hpp"
#include "window_text.hpp"
#include "screen_reset_error.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "radio_button.hpp"

class ScreenHelpFWUpdate : public AddSuperWindow<screen_t> {
    window_header_t header;
    window_text_t description;
#if defined(USE_ST7789)
    window_text_t description2;
#endif
    window_text_t help;
    window_qr_t qr;
    RadioButton radio;

public:
    ScreenHelpFWUpdate();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
