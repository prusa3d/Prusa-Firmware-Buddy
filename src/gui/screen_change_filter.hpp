/// @file
#pragma once

#include "gui.hpp"
#include "screen.hpp"
#include "window_text.hpp"
#include "window_header.hpp"
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
};
