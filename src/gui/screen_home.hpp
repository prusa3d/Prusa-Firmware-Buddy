//screen_home.hpp
#pragma once
#include "window_header.hpp"
#include "status_footer.h"
#include "gui.hpp"

struct screen_home_data_t : public window_frame_t {
    window_header_t header;
    status_footer_t footer;

    window_icon_t logo;
    window_icon_button_t w_buttons[6];
    window_text_t w_labels[6];

    uint8_t is_starting;
    uint32_t time;

    screen_home_data_t();

private:
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
    virtual void draw() override;

    void printBtnEna();
    void printBtnDis();
};
