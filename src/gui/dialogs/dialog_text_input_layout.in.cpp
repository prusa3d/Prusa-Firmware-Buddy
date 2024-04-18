#pragma once

#include "dialog_text_input.hpp"

#include <array>
#include <variant>
#include <img_resources.hpp>

namespace dialog_text_input {

enum class SpecialButton : uint8_t {
    ok,
    cancel,
    clear,
    backspace,
    space,

    uppercase,
    lowercase,
    symbols,
    numbers,

    _cnt
};

static constexpr char special_button_code = static_cast<char>(255);

// One extra char for the terminating \0
struct ButtonRec : std::array<char, 5> {

    constexpr ButtonRec() = default;
    constexpr ButtonRec(const ButtonRec &) = default;
    constexpr ButtonRec(SpecialButton b)
        : array { special_button_code, static_cast<char>(b) } {}

    constexpr bool is_special() const {
        return at(0) == special_button_code;
    }
    constexpr SpecialButton to_special_button() const {
        return is_special() ? static_cast<SpecialButton>(at(1)) : SpecialButton::_cnt;
    }
};

using ButtonsRow = std::array<ButtonRec, button_cols>;

// Cannot use "using ButtonsLayout = ...;" because of the previous forward declaration (dumb GCC)
struct ButtonsLayout : public std::array<ButtonsRow, button_rows> {};

constexpr ButtonRec operator""_b(const char *str, size_t) {
    ButtonRec r;
    auto *rp = r.data();
    while (*str) {
        *rp++ = *str++;
    }

    // Make sure we've left the termiating zero in the array
    if (rp >= r.end()) {
        std::terminate();
    }

    return r;
}

constexpr EnumArray<SpecialButton, std::variant<const char *, const img::Resource *>, SpecialButton::_cnt> special_button_labels {
    { SpecialButton::ok, &img::ok_60x60 },
    { SpecialButton::cancel, &img::cancel_60x60 },
    { SpecialButton::clear, &img::clear_60x60 },
    { SpecialButton::backspace, &img::backspace_60x60 },
    { SpecialButton::space, &img::space_60x60 },

    { SpecialButton::uppercase, "ABC" },
    { SpecialButton::lowercase, "abc" },
    { SpecialButton::symbols, "!.$" },
    { SpecialButton::numbers, "123" },
};

constexpr ButtonRec quick_symbols_button = ".-@_"_b;

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

constexpr ButtonsLayout layout_text_uppercase = [] {
    ButtonsLayout r = layout_text_lowercase;
    for (auto &row : r) {
        for (auto &rec : row) {
            if (rec == ButtonRec(SpecialButton::uppercase)) {
                rec = SpecialButton::lowercase;

            } else if (!rec.is_special()) {
                for (char &ch : rec) {
                    // Poor man's toupper
                    if (ch >= 'a' && ch <= 'z') {
                        ch += 'A' - 'a';
                    }
                }
            }
        }
    }

    return r;
}();

constexpr ButtonsLayout layout_symbols = {
    ButtonsRow {
        SpecialButton::numbers,
        ".,?!"_b,
        ":;"_b,
        "-_"_b,
        "+*"_b,
        SpecialButton::backspace,
    },
    ButtonsRow {
        SpecialButton::lowercase,
        "/|"_b,
        "%&#"_b,
        "\'\""_b,
        "=~"_b,
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

} // namespace dialog_text_input
