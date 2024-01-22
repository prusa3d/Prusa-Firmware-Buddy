/**
 * @file footer_text.cpp
 * @author Radek Vana
 * @date 2021-03-31
 */

#include "footer_text.hpp"
#include "guitypes.hpp"
#include <guiconfig/GuiDefaults.hpp>

FooterText::FooterText(window_t *parent, Rect16::Left_t left, string_view_utf8 txt)
    : AddSuperWindow<WindowBlinkingText>(
        parent,
        [parent, left] {
            if (!parent) {
                return Rect16(); // does not have parrent, cannot calculate rect
            }

            size_ui16_t sz = parent->GetRect().Size();
            point_i16_t pt = { left, int16_t(std::max((sz.h - height(GuiDefaults::FooterFont)) / 2, 0)) };
            sz.w -= left;

            return Rect16(pt, sz);
        }(),
        txt) {
    set_font(GuiDefaults::FooterFont);
}
