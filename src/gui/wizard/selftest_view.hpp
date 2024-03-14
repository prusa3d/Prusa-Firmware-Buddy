/**
 * @file selftest_view.hpp
 * @author Radek Vana
 * @brief Window showing entire result of selftest
 * Containing selftest groups - one per test, failed ones are shown first
 * @date 2022-01-21
 */

#pragma once

#include "selftest_group.hpp"
#include "window.hpp"

class SelfTestView : public AddSuperWindow<window_t> {
    SelfTestGroup *first_failed;
    SelfTestGroup *first_passed;
    size_t count;
    Rect16::Height_t height_of_all_items;
    Rect16::Height_t height_draw_offset;

    SelfTestGroup *getLastFailed() const;
    SelfTestGroup *getLastPassed() const;
    SelfTestGroup *getFirst() const;
    SelfTestGroup *getNext(const SelfTestGroup &currnet) const; // can switch from failed to passed list

    void addFailed(SelfTestGroup &failed);
    void addPassed(SelfTestGroup &passed);
    Rect16 calculateRect(int shift, Rect16::Height_t px_drawn) const;

    static constexpr Rect16::Height_t test_gap = 10; // gap between tests
public:
    SelfTestView(window_t *parrent, Rect16 rc);
    void Add(SelfTestGroup &group);

    // cannot use knob event directly, need to provide position api
    // to be able to to synchronize with side bar
    Rect16::Height_t GetTotalHeight() const { return height_of_all_items; }
    void SetDrawOffset(Rect16::Height_t offset);
    Rect16::Height_t GetDrawOffset() const { return height_draw_offset; }

protected:
    virtual void unconditionalDraw() override;
};
