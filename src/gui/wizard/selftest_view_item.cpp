/**
 * @file selftest_view_item.cpp
 * @author Radek Vana
 * @date 2022-01-21
 */

#include "selftest_view_item.hpp"
#include "wizard_config.hpp"

SelfTestViewItem::SelfTestViewItem(Rect16::Height_t h)
    : next(nullptr)
    , height(h) {}

Rect16 SelfTestViewItem::Rect(Rect16::Top_t top) const {
    return Rect16(WizardDefaults::MarginLeft, top, WizardDefaults::X_space, height);
}
