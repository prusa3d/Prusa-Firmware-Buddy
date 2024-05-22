/**
 * @file selftest_group.cpp
 * @author Radek Vana
 * @date 2022-01-21
 */

#include "selftest_group.hpp"

SelfTestGroup::SelfTestGroup(string_view_utf8 txt)
    : next(nullptr)
    , first(nullptr)
    , name(txt, is_multiline::no)
    , failed(false) {
    Add(name);
    Add(separator);
}

void SelfTestGroup::Draw(Rect16 rc, int dontdraw_first_n_px) const {
    SelfTestViewItem *item = first;
    while (item) {
        if (dontdraw_first_n_px > 0) {
            dontdraw_first_n_px -= item->GetHeight();
        } else {
            // apply offset to rect
            dontdraw_first_n_px = -dontdraw_first_n_px;
            if (dontdraw_first_n_px > rc.Height()) {
                return; // cannot apply offset to rect
            }
            rc += Rect16::Top_t(dontdraw_first_n_px);
            rc -= Rect16::Height_t(dontdraw_first_n_px);
            dontdraw_first_n_px = 0;

            if (item->GetHeight() > rc.Height()) {
                return; // item does not fit
            }

            item->Draw(rc.Top());
            rc += Rect16::Top_t(item->GetHeight());
            rc -= item->GetHeight();
        }
        item = item->GetNext();
    }
}

Rect16::Height_t SelfTestGroup::GetHeight() const {
    SelfTestViewItem *item = first;
    Rect16::Height_t h = 0;
    while (item) {
        h = h + item->GetHeight();
        item = item->GetNext();
    }
    return h;
}

void SelfTestGroup::Add(SelfTestViewItem &item) {
    if (!first) {
        first = &item;
        return;
    }

    SelfTestViewItem *last = first;
    while (last->GetNext()) {
        last = last->GetNext();
        if (last == &item) { // Trying to add item that is already present
            return;
        }
    }
    last->SetNext(&item);
}

void SelfTestGroup::Remove(SelfTestViewItem &item) {
    if (!first) {
        return;
    }

    if (first == &item) {
        first = item.GetNext();
        item.SetNext(nullptr);
        return;
    }

    SelfTestViewItem *last = first;
    while (last) {
        if (last == &item) {
            last->SetNext(item.GetNext());
            item.SetNext(nullptr);
            return;
        }

        last = last->GetNext();
    }
}
