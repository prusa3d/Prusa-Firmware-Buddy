#pragma once

#include "dialog_text_input_layout.in.cpp"

#include <array>

#define HAS_NUMBERS_LAYOUT() 0

namespace dialog_text_input {

constexpr int16_t button_padding = 2;
constexpr int16_t buttons_rect_margin = 4;

constexpr ButtonsLayout layout_text_lowercase = {
    ButtonsRow {
        ".,?!"_b,
        "abc"_b,
        "def"_b,
        SpecialButton::backspace,
    },
    ButtonsRow {
        "ghi"_b,
        "jkl"_b,
        "mno"_b,
        SpecialButton::clear,
    },
    ButtonsRow {
        "pqrs"_b,
        "tuv"_b,
        "wxyz"_b,
        SpecialButton::space,
    },
    ButtonsRow {
        "123"_b,
        "456"_b,
        "789"_b,
        SpecialButton::uppercase,
    },
    ButtonsRow {
        SpecialButton::cancel,
        SpecialButton::symbols,
        "0+-"_b,
        SpecialButton::ok,
    },
};

constexpr ButtonsLayout layout_symbols = {
    ButtonsRow {
        ".,"_b,
        "?!"_b,
        ":;"_b,
        SpecialButton::backspace,
    },
    ButtonsRow {
        "+-"_b,
        "*/"_b,
        "|&"_b,
        SpecialButton::clear,
    },
    ButtonsRow {
        "%#"_b,
        "$@"_b,
        "_=~"_b,
        SpecialButton::space,
    },
    ButtonsRow {
        "()"_b,
        "[]"_b,
        "{}"_b,
        SpecialButton::lowercase,
    },
    ButtonsRow {
        SpecialButton::cancel,
        "<>"_b,
        "\"'"_b,
        SpecialButton::ok,
    },
};

constexpr ButtonsLayout layout_text_uppercase = to_uppercase_layout(layout_text_lowercase);

constexpr std::array all_button_layouts {
    &layout_text_lowercase,
    &layout_text_uppercase,
    &layout_symbols
};

} // namespace dialog_text_input
