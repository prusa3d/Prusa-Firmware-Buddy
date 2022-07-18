/**
 * @file selftest_view_item_text.cpp
 * @author Radek Vana
 * @date 2022-01-21
 */

#include "selftest_view_item_text.hpp"
#include "window_icon.hpp"  // CalculateMinimalSize
#include "display_helper.h" // font_meas_text
#include "GuiDefaults.hpp"
#include "resource.h"

SelfTestViewText::SelfTestViewText(string_view_utf8 txt, is_multiline multiln)
    : SelfTestViewItem(Rect16::Height_t(font_meas_text(GuiDefaults::Font, &txt, &strlen_text_dummy).h))
    , text(txt)
    , multiline(multiln) {}

void SelfTestViewText::Draw(Rect16::Top_t top) const {
    Rect16 rc = Rect(top);
    render(rc);
}

/*****************************************************************************/
//SelfTestViewText
static constexpr size_t icon_width = 16;
static constexpr size_t text_pos_after_icon = WizardDefaults::col_after_icon - WizardDefaults::col_0;

void SelfTestViewText::render(Rect16 rc) const {
    // TODO use some function not changing background, just draw text
    // background is guaranted to be clear
    render_text_align(rc, text, GuiDefaults::Font, GuiDefaults::ColorBack, GuiDefaults::MenuColorText,
        padding_ui8_t { 0, 0, 0, 0 }, { Align_t::LeftTop(), multiline });
}

SelfTestViewTextWithIcon::SelfTestViewTextWithIcon(string_view_utf8 txt, ResourceId icon_id, is_multiline multiln)
    : SelfTestViewText(txt, multiln)
    , icon_id(icon_id)
    , icon_sz(window_icon_t::CalculateMinimalSize(icon_id)) {}

void SelfTestViewTextWithIcon::Draw(Rect16::Top_t top) const {
    Rect16 rc = Rect(top);
    render_icon_align(Rect16(rc.TopLeft(), icon_sz), icon_id, GuiDefaults::ColorBack, Align_t::LeftTop());
    rc += Rect16::Left_t(text_pos_after_icon);
    rc -= Rect16::Width_t(text_pos_after_icon);
    render(rc);
}

/*****************************************************************************/
//SelfTestViewTextWithIconAndResult
static constexpr size_t status_icon_width = WizardDefaults::status_icon_w;

SelfTestViewTextWithIconAndResult::SelfTestViewTextWithIconAndResult(string_view_utf8 txt, ResourceId icon_id, TestResult_t result, is_multiline multiln)
    : SelfTestViewTextWithIcon(txt, icon_id, multiln)
    , icon_result_id(ResultToIconId(result)) {}

void SelfTestViewTextWithIconAndResult::Draw(Rect16::Top_t top) const {
    Rect16 rc = Rect(top);
    render_icon_align(Rect16(rc.TopLeft(), icon_sz), icon_id, GuiDefaults::ColorBack, Align_t::LeftTop());
    rc += Rect16::Left_t(text_pos_after_icon);
    rc -= Rect16::Width_t(text_pos_after_icon + status_icon_width);
    render(rc);
    render_icon_align(Rect16(WizardDefaults::status_icon_X_pos, rc.Top(), icon_sz.w, icon_sz.h), icon_result_id, GuiDefaults::ColorBack, Align_t::LeftTop());
}

ResourceId SelfTestViewTextWithIconAndResult::ResultToIconId(TestResult_t res) {
    switch (res) {
    case TestResult_t::Passed:
        return IDR_PNG_ok_color_18px;
    case TestResult_t::Failed:
        return IDR_PNG_nok_color_18px;
    default:
        break;
    }
    return IDR_PNG_dash_18px;
}
