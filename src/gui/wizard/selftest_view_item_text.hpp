/**
 * @file selftest_view_item_text.hpp
 * @author Radek Vana
 * @brief Text drawn in selftest result, could be test name or explanation why selftest failed
 * @date 2022-01-21
 */

#pragma once

#include "selftest_view_item.hpp"
#include "string_view_utf8.hpp"
#include "selftest_result_type.hpp"
#include "font_flags.hpp" // is_multiline

class SelfTestViewText : public SelfTestViewItem {
    string_view_utf8 text;
    is_multiline multiline;

protected:
    void render(Rect16 rc) const;
    static Rect16::Height_t CalculateHeight(string_view_utf8 &txt, is_multiline multiln, Rect16::Width_t width);

public:
    SelfTestViewText(string_view_utf8 txt, is_multiline multiln = is_multiline::no, Rect16::Width_t width = 0);
    virtual void Draw(Rect16::Top_t top) const override;
};

class SelfTestViewTextWithIcon : public SelfTestViewText {
protected:
    const img::Resource *icon;

public:
    SelfTestViewTextWithIcon(string_view_utf8 txt, const img::Resource *icon, is_multiline multiln = is_multiline::no, Rect16::Width_t width = 0);
    virtual void Draw(Rect16::Top_t top) const override;
};

class SelfTestViewTextWithIconAndResult : public SelfTestViewTextWithIcon {
    const img::Resource *icon_result;

public:
    SelfTestViewTextWithIconAndResult(string_view_utf8 txt, const img::Resource *icon, TestResult result, is_multiline multiln = is_multiline::no, Rect16::Width_t width = 0);
    virtual void Draw(Rect16::Top_t top) const override;
    static const img::Resource *ResultToIconId(TestResult res);
    void SetState(TestResult res);
};
