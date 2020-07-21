//screen_qr_error.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "support_utils.h" //MAX_LEN_4QR
#include <array>

struct screen_qr_error_data_t : public window_frame_t {
    window_text_t errText;
    window_text_t errDescription;
    window_text_t info;
    window_qr_t qr;
    std::array<char, MAX_LEN_4QR + 1> qr_text;
    bool first_run_flag;

public:
    screen_qr_error_data_t();

private:
    virtual int windowEvent(window_t *sender, uint8_t event, void *param) override;
    virtual void unconditionalDraw() override;
};
