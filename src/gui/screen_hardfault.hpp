// screen_hardfault.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "screen_reset_error.hpp"

class screen_hardfault_data_t : public AddSuperWindow<screen_reset_error_data_t> {
    window_text_t text;
    window_text_t exit;

public:
    screen_hardfault_data_t();

protected:
    virtual void draw() override;
};
