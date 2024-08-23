#pragma once

#include "error_codes.hpp"
#include "window_text.hpp"
#include <array>

class TextErrorUrlWindow : public window_text_t {
private:
    std::array<char, 24> buffer;

public:
    TextErrorUrlWindow(window_t *parent, Rect16 rect, ErrCode ec);

    void set_error_code(ErrCode ec);
};
