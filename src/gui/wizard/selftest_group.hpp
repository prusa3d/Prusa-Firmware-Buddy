/**
 * @file selftest_group.hpp
 * @author Radek Vana
 * @brief Part containing all information about one subtest (fans, axis ...)
 * @date 2022-01-21
 */

#pragma once

#include "selftest_view_item.hpp"
#include "selftest_view_item_separator.hpp"
#include "selftest_view_item_text.hpp"

class SelfTestGroup {
    SelfTestGroup *next;
    SelfTestViewItem *first;

    // every test has name folowed by separator
    SelfTestViewText name;
    SelfTestViewSeparator separator;

protected:
    bool failed;

public:
    SelfTestGroup(string_view_utf8 txt);
    Rect16::Height_t GetHeight() const;

    void Draw(Rect16 rc, int dontdraw_first_n_px) const;
    void Add(SelfTestViewItem &item);

    /**
     * @brief Remove an item from the group.
     * @param item remove this
     */
    void Remove(SelfTestViewItem &item);

    SelfTestGroup *GetNext() const { return next; }
    void SetNext(SelfTestGroup *nxt) { next = nxt; }
    bool Failed() const { return failed; }
};
