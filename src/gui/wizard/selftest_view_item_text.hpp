/**
 * @file selftest_view_item_text.hpp
 * @author Radek Vana
 * @brief Text drawn in selftest result, could be test name or explanation why selftest failed
 * @date 2022-01-21
 */

#pragma once

#include "selftest_view_item.hpp"
#include "string_view_utf8.hpp"
#include "selftest_eeprom.hpp"
#include "font_flags.hpp" // is_multiline

class SelfTestViewText : public SelfTestViewItem {
    string_view_utf8 text;
    is_multiline multiline;
    uint16_t strlen_text_dummy; // dummy variable to pass its addres to font_meas_text in ctor
protected:
    void render(Rect16 rc) const;

public:
    SelfTestViewText(string_view_utf8 txt, is_multiline multiln = is_multiline::no);
    virtual void Draw(Rect16::Top_t top) const override;
};

class SelfTestViewTextWithIcon : public SelfTestViewText {
protected:
    ResourceId icon_id;
    size_ui16_t icon_sz;

public:
    SelfTestViewTextWithIcon(string_view_utf8 txt, ResourceId icon_id, is_multiline multiln = is_multiline::no);
    virtual void Draw(Rect16::Top_t top) const override;
};

class SelfTestViewTextWithIconAndResult : public SelfTestViewTextWithIcon {
    ResourceId icon_result_id;

public:
    SelfTestViewTextWithIconAndResult(string_view_utf8 txt, ResourceId icon_id, TestResult_t result, is_multiline multiln = is_multiline::no);
    virtual void Draw(Rect16::Top_t top) const override;
    static ResourceId ResultToIconId(TestResult_t res);
};
