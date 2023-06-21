/**
 * @file i_window_text.hpp
 * @author Radek Vana
 * @brief parent of text and number
 * @date 2021-10-07
 */

#pragma once

#include "window.hpp"

class IWindowText : public AddSuperWindow<window_aligned_t> {
    // depends on color_scheme_foreground flag
    // if enabled and set != nullptr
    //   window automatically draws differently when selected or shadowed
    union {
        color_t color_text;
        const color_scheme *pTextColorScheme;
    };

    font_t *font;

public:
    padding_ui8_t padding; // TODO private

    void SetTextColor(color_t clr);
    void SetTextColor(const color_scheme &clr);
    color_t GetTextColor() const;
    void set_font(font_t *val);
    const font_t *get_font() const;
    void SetPadding(padding_ui8_t padd);

    IWindowText(window_t *parent, Rect16 rect, is_closed_on_click_t close = is_closed_on_click_t::no);
};
