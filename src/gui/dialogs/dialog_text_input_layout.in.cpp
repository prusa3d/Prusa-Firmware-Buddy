#pragma once

#include <array>
#include <variant>
#include <cstdint>
#include <cstddef>
#include <exception>

#include "dialog_text_input_layout.hpp"

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
    constexpr bool is_character_emitting() const {
        return !is_special() || to_special_button() == SpecialButton::space;
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

constexpr ButtonsLayout to_uppercase_layout(ButtonsLayout layout) {
    for (auto &row : layout) {
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

    return layout;
}

} // namespace dialog_text_input

#if HAS_LARGE_DISPLAY()
    #include "dialog_text_input_layout_xlcd.in.cpp"
#else
    #include "dialog_text_input_layout_mini.in.cpp"
#endif
