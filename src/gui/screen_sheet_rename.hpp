#pragma once

#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.h"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "ScreenFactory.hpp"

struct screen_sheet_rename_t : public window_frame_t {
    static void index(std::uint32_t);
    screen_sheet_rename_t();
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
    static std::uint32_t index_;

private:
    window_header_t header;
    status_footer_t footer;
    window_text_button_t button_ok;
    window_text_button_t button_cancel;
    window_text_t text_name;
};
