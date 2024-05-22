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
#include <guiconfig/guiconfig.h>

class ScreenHelpFWUpdate : public screen_t {
    window_header_t header;
    window_text_t description;
#if HAS_MINI_DISPLAY()
    window_text_t description2;
#endif
    window_text_t help;
    window_qr_t qr;
    RadioButton radio;

    std::array<char, 128> help_text;
    std::array<char, 32> qr_link_text;

public:
    ScreenHelpFWUpdate();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
