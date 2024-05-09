#pragma once

#include "dialog_text_input_layout.in.cpp"

#define HAS_NUMBERS_LAYOUT() 1

namespace dialog_text_input {

constexpr int16_t button_padding = 4;
constexpr int16_t buttons_rect_margin = 8;

constexpr ButtonsLayout layout_text_lowercase = {
    ButtonsRow {
        SpecialButton::numbers,
        ".,?!"_b,
        "abc"_b,
        "def"_b,
        "-_@"_b,
        SpecialButton::backspace,
    },
    ButtonsRow {
        SpecialButton::uppercase,
        "ghi"_b,
        "jkl"_b,
        "mno"_b,
        "()[]"_b,
        SpecialButton::clear,
    },
    ButtonsRow {
        SpecialButton::cancel,
        "pqrs"_b,
        "tuv"_b,
        "wxyz"_b,
        SpecialButton::space,
        SpecialButton::ok,
    },
};

constexpr ButtonsLayout layout_symbols = {
    ButtonsRow {
        SpecialButton::numbers,
        ".,?!"_b,
        ":;"_b,
        "\'\""_b,
        "@$"_b,
        SpecialButton::backspace,
    },
    ButtonsRow {
        SpecialButton::lowercase,
        "&|"_b,
        "+-*/"_b,
        "_=~"_b,
        "%#"_b,
        SpecialButton::clear,
    },
    ButtonsRow {
        SpecialButton::cancel,
        "()"_b,
        "[]"_b,
        "{}"_b,
        "<>"_b,
        SpecialButton::ok,
    },
};

constexpr ButtonsLayout layout_numbers = {
    ButtonsRow {
        SpecialButton::symbols,
        "7"_b,
        "8"_b,
        "9"_b,
        "-+%"_b,
        SpecialButton::backspace,
    },
    ButtonsRow {
        SpecialButton::lowercase,
        "4"_b,
        "5"_b,
        "6"_b,
        ".,"_b,
        SpecialButton::clear,
    },
    ButtonsRow {
        SpecialButton::cancel,
        "1"_b,
        "2"_b,
        "3"_b,
        "0"_b,
        SpecialButton::ok,
    },
};

constexpr ButtonsLayout layout_text_uppercase = to_uppercase_layout(layout_text_lowercase);

constexpr std::array all_button_layouts {
    &layout_text_lowercase,
    &layout_text_uppercase,
    &layout_symbols,
    &layout_numbers,
};

} // namespace dialog_text_input
