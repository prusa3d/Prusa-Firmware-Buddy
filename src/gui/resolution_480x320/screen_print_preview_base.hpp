/**
 * @file screen_print_preview_base.hpp
 */
#pragma once
#include "screen.hpp"
#include "window_header.hpp"
#include "radio_button_preview.hpp"
#include "window_roll_text.hpp"

class ScreenPrintPreviewBase : public screen_t {
protected:
    window_header_t header;
#if not PRINTER_IS_PRUSA_MINI()
    char title_text_text[FILE_PATH_BUFFER_LEN];
#endif
    window_roll_text_t title_text;
    RadioButtonPreview radio; // shows 2 mutually exclusive buttons Print and Back

public:
    ScreenPrintPreviewBase();
};
