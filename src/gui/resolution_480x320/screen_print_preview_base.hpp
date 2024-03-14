/**
 * @file screen_print_preview_base.hpp
 */
#pragma once
#include "screen.hpp"
#include "window_header.hpp"
#include "radio_button_preview.hpp"
#include "window_roll_text.hpp"

class ScreenPrintPreviewBase : public AddSuperWindow<screen_t> {
protected:
    window_header_t header;
    window_roll_text_t title_text;
    RadioButtonPreview radio; // shows 2 mutually exclusive buttons Print and Back

public:
    static constexpr const char *txt_wrong_fil_type = N_("A filament specified in the G-code is either not loaded or wrong type.");

    ScreenPrintPreviewBase();
};
