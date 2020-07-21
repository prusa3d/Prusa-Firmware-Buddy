//screen_printing_serial.hpp
#pragma once
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.h"

enum class buttons_t {
    TUNE = 0,
    PAUSE,
    DISCONNECT,
    count
};

struct screen_printing_serial_data_t : public window_frame_t {
    window_header_t header;
    status_footer_t footer;

    window_icon_t octo_icon;

    window_icon_t w_buttons[static_cast<size_t>(buttons_t::count)];
    window_text_t w_labels[static_cast<size_t>(buttons_t::count)];

    int last_tick;
    bool disconnect_pressed;

public:
    screen_printing_serial_data_t();
    ~screen_printing_serial_data_t();

private:
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
    virtual void unconditionalDraw() override;
    void DisableButton(buttons_t b);
};
