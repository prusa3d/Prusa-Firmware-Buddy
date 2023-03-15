#pragma once

#include "window_header.hpp"
#include "status_footer.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "screen.hpp"

struct screen_sheet_rename_t : public screen_t {
    static void index(uint32_t);
    screen_sheet_rename_t();
    static uint32_t index_;

private:
    window_header_t header;
    StatusFooter footer;
    window_text_button_t button_ok;
    window_text_button_t button_cancel;
    window_text_t text_name;
};
