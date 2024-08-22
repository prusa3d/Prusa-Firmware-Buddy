/**
 * @file screen_filter_change.hpp
 */
#pragma once
#include "gui.hpp"
#include "screen.hpp"
#include "window_text.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "radio_button.hpp"
#include <gui/qr.hpp>

class ScreenChangeFilter : public screen_t {
    window_header_t header;
    window_text_t description;
    window_text_t help;
    QRStaticStringWindow qr;
    RadioButton radio;

public:
    ScreenChangeFilter();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
