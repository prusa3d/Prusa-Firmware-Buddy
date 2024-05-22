/**
 * @file selftest_view.cpp
 * @author Radek Vana
 * @date 2022-01-21
 */

#include "selftest_view.hpp"

SelfTestView::SelfTestView(window_t *parrent, Rect16 rc)
    : AddSuperWindow<window_t>(parrent, rc)
    , first_failed(nullptr)
    , first_passed(nullptr)
    , count(0)
    , height_of_all_items(0)
    , height_draw_offset(0) {
}

/**
 * @brief returns last failed SelfTestGroup
 * it is the one just before first passed
 * @return SelfTestGroup*
 */
SelfTestGroup *SelfTestView::getLastFailed() const {
    SelfTestGroup *item = first_failed;
    while (item && item->GetNext()) {
        item = item->GetNext();
    }
    return item;
}

/**
 * @brief returns last passed SelfTestGroup
 * its pointer to next points to null
 * @return SelfTestGroup*
 */
SelfTestGroup *SelfTestView::getLastPassed() const {
    SelfTestGroup *item = first_passed;
    while (item && item->GetNext()) {
        item = item->GetNext();
    }
    return item;
}

SelfTestGroup *SelfTestView::getFirst() const {
    if (first_failed) {
        return first_failed;
    }
    return first_passed;
}

SelfTestGroup *SelfTestView::getNext(const SelfTestGroup &currnet) const {
    bool failed = currnet.Failed();
    SelfTestGroup *ret = currnet.GetNext();
    if (ret) {
        return ret;
    }

    return failed ? first_passed : nullptr;
}

/**
 * @brief adds group
 * failed groups are before passed ones
 * @param group
 */
void SelfTestView::Add(SelfTestGroup &group) {
    if (group.Failed()) {
        addFailed(group);
    } else {
        addPassed(group);
    }
    height_of_all_items = height_of_all_items + group.GetHeight();
}

/**
 * @brief adds a failed group
 * failed groups are before passed ones
 * @param failed
 */
void SelfTestView::addFailed(SelfTestGroup &failed) {
    SelfTestGroup *last = getLastFailed();
    if (last) {
        last->SetNext(&failed);
    } else {
        first_failed = &failed;
    }
    ++count;
}

/**
 * @brief add passed group
 * passed groups are after failed ones
 * @param passed
 */
void SelfTestView::addPassed(SelfTestGroup &passed) {
    SelfTestGroup *last = getLastPassed();
    if (last) {
        last->SetNext(&passed);
    } else {
        first_passed = &passed;
    }
    ++count;
}

void SelfTestView::unconditionalDraw() {
    SelfTestGroup *item = getFirst();
    Rect16 rc = GetRect();

    int shift = height_draw_offset;

    while (item && rc.Height() > 0) {
        // calculate height
        Rect16::Height_t h = std::min(rc.Height(), Rect16::Height_t(item->GetHeight() + test_gap));

        if ((shift >= h)) {
            shift -= h;
            item = getNext(*item);
            continue;
        }

        if (shift < 0) {
            shift = 0;
        }
        h = h - shift; // part of item is shifted away from visible rectangle

        // clear background
        Rect16 item_rc = rc;
        item_rc = h;
        display::FillRect(item_rc, GetBackColor());

        // cut off used part of rect
        rc -= h;
        rc += Rect16::Top_t(h);

        item->Draw(item_rc, shift);
        shift -= int(item->GetHeight() + test_gap);
        item = getNext(*item);
    }

    // fill rest
    if (rc.Height()) {
        display::FillRect(rc, GetBackColor());
    }
}

void SelfTestView::SetDrawOffset(Rect16::Height_t offset) {
    if (height_of_all_items == 0) {
        return;
    }
    offset = (offset >= height_of_all_items) ? Rect16::Height_t(height_of_all_items - 1) : offset;
    if (offset == height_draw_offset) {
        return; // do not invalidate
    }
    height_draw_offset = offset;
    Invalidate();
}
