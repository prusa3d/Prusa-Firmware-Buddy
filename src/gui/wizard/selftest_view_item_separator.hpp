/**
 * @file selftest_view_item_separator.hpp
 * @author Radek Vana
 * @brief Separator line to be drawn in selftest result
 * @date 2022-01-21
 */

#pragma once

#include "selftest_view_item.hpp"

class SelfTestViewSeparator : public SelfTestViewItem {
public:
    SelfTestViewSeparator();
    virtual void Draw(Rect16::Top_t top) const override;
};
