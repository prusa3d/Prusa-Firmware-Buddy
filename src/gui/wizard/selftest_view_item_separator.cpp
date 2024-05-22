/**
 * @file selftest_view_item_separator.cpp
 * @author Radek Vana
 * @date 2022-01-21
 */

#include "selftest_view_item_separator.hpp"
#include <guiconfig/wizard_config.hpp>

constexpr Rect16::Height_t line_height = WizardDefaults::progress_h;
constexpr Rect16::Height_t top_padding = 2;
constexpr Rect16::Height_t bot_padding = 5;

SelfTestViewSeparator::SelfTestViewSeparator()
    : SelfTestViewItem(line_height + top_padding + bot_padding) {}

void SelfTestViewSeparator::Draw(Rect16::Top_t top) const {
    Rect16 rc = Rect(top);
    rc = top_padding;
    display::FillRect(rc, GuiDefaults::ColorBack);
    rc += Rect16::Top_t(rc.Height());
    rc = Rect16::Height_t(line_height);
    display::FillRect(rc, COLOR_ORANGE);
    rc += Rect16::Top_t(rc.Height());
    rc = bot_padding;
    display::FillRect(rc, GuiDefaults::ColorBack);
}
