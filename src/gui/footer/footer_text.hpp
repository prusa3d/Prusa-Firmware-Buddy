/**
 * @file footer_text.hpp
 * @author Radek Vana
 * @brief text used in footer
 * @date 2021-03-31
 */

#pragma once

#include "window_text.hpp"

class FooterText : public WindowBlinkingText {
public:
    FooterText(window_t *parent, Rect16::Left_t left, const string_view_utf8 &txt = string_view_utf8::MakeNULLSTR());
};
