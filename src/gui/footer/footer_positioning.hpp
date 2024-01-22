#pragma once
/**
 * @file footer_positioning.hpp
 * @author Radek Vana
 * @brief calculations of footer rectangles
 * @date 2021-04-14
 */
#include <guiconfig/GuiDefaults.hpp>

namespace footer {
// line number from 0
// 0 is at bottom
constexpr Rect16 line_rect(size_t line_no) {
    Rect16 ret = GuiDefaults::RectFooter;
    ret += Rect16::Left_t(GuiDefaults::FooterPadding.left);
    ret -= Rect16::Width_t(GuiDefaults::FooterPadding.left + GuiDefaults::FooterPadding.right);
    ret = GuiDefaults::FooterItemHeight;
    ret += Rect16::Top_t(GuiDefaults::RectFooter.Height() - GuiDefaults::FooterPadding.bottom
        - (line_no) * (GuiDefaults::FooterLinesSpace + GuiDefaults::FooterItemHeight) - GuiDefaults::FooterItemHeight);

    return ret;
}

} // namespace footer
