/**
 * @file footer_positioning.hpp
 * @author Radek Vana
 * @brief calculations of footer rectangles
 * @date 2021-04-14
 */
#include "GuiDefaults.hpp"

namespace footer {
// line number from 0
constexpr Rect16 LineRect(size_t line_no) {
    Rect16 ret = GuiDefaults::RectFooter;
    ret += Rect16::Left_t(GuiDefaults::FooterPadding.left);
    ret -= Rect16::Width_t(GuiDefaults::FooterPadding.left + GuiDefaults::FooterPadding.right);
    ret = GuiDefaults::FooterItemHeight;
    ret += Rect16::Top_t(GuiDefaults::FooterPadding.top + line_no * (GuiDefaults::FooterLinesSpace + GuiDefaults::FooterItemHeight));

    return ret;
}

}
