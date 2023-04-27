/**
 * @file selftest_view_item.hpp
 * @author Radek Vana
 * @brief Single selftest view item (could be test name, separator, result of subtest - test of one axis, etc.)
 * @date 2022-01-21
 */

#pragma once

#include "Rect16.h"

class SelfTestViewItem {
protected:
    SelfTestViewItem *next;
    Rect16::Height_t height;

public:
    virtual ~SelfTestViewItem() = default;
    Rect16 Rect(Rect16::Top_t top) const;

    SelfTestViewItem(Rect16::Height_t h);
    virtual void Draw(Rect16::Top_t top) const = 0;

    Rect16::Height_t GetHeight() const { return height; }
    SelfTestViewItem *GetNext() const { return next; }
    void SetNext(SelfTestViewItem *nxt) { next = nxt; }
};
