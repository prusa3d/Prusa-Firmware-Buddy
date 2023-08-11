/**
 * @file selftest_view_item_text.cpp
 * @author Radek Vana
 * @date 2022-01-21
 */

#include "selftest_view_item_text.hpp"
#include "display_helper.h" // font_meas_text
#include "GuiDefaults.hpp"
#include "wizard_config.hpp"
#include "img_resources.hpp"

/**
 * @brief Construct a new Self Test View Text:: Self Test View Text object
 *
 * if multiln == is_multiline::no, width has no effect
 * if multiln == is_multiline::yes && width == 0, full screen width width is used
 *
 * @param txt       text
 * @param multiln   multiline flag
 * @param width     width to be able to measure height (number of lines) needed for multiline text
 */
SelfTestViewText::SelfTestViewText(string_view_utf8 txt, is_multiline multiln, Rect16::Width_t width)
    : SelfTestViewItem(CalculateHeight(txt, multiln, width == 0 ? Rect16::Width_t(GuiDefaults::ScreenWidth) : width))
    , text(txt)
    , multiline(multiln) {}

void SelfTestViewText::Draw(Rect16::Top_t top) const {
    Rect16 rc = Rect(top);
    render(rc);
}

static constexpr size_t text_pos_after_icon = WizardDefaults::col_after_icon - WizardDefaults::col_0;

Rect16::Height_t SelfTestViewText::CalculateHeight(string_view_utf8 &txt, is_multiline multiln, Rect16::Width_t width) {
    if (!GuiDefaults::Font) {
        return Rect16::Height_t(0);
    }

    if (multiln == is_multiline::no) {
        return GuiDefaults::Font->h;
    }

    std::optional<size_ui16_t> ret = font_meas_text(*GuiDefaults::Font, txt, width);
    return ret ? Rect16::Height_t(ret->h) : Rect16::Height_t(0);
}

void SelfTestViewText::render(Rect16 rc) const {
    // TODO use some function not changing background, just draw text
    // background is guaranted to be clear
    render_text_align(rc, text, GuiDefaults::Font, GuiDefaults::ColorBack, GuiDefaults::MenuColorText,
        padding_ui8_t { 0, 0, 0, 0 }, { Align_t::LeftTop(), multiline });
}

/**
 * @brief Construct a new Self Test View Text With Icon:: Self Test View Text With Icon object
 *
 * if multiln == is_multiline::no, width has no effect
 * if multiln == is_multiline::yes && width == 0, full screen width is used (minus icon width and space between icon and text)
 * if multiln == is_multiline::yes && width != 0, icon width and space between icon and text is subtracted from width
 *
 * @param txt       text
 * @param icon      icon
 * @param multiln   multiline flag
 * @param width     width to be able to measure height (number of lines) needed for multiline text
 */
SelfTestViewTextWithIcon::SelfTestViewTextWithIcon(string_view_utf8 txt, const img::Resource *icon, is_multiline multiln, Rect16::Width_t width)
    : SelfTestViewText(txt, multiln, width == 0 ? Rect16::Width_t(GuiDefaults::ScreenWidth) : Rect16::Width_t(width - text_pos_after_icon))
    , icon(icon) {}

void SelfTestViewTextWithIcon::Draw(Rect16::Top_t top) const {
    Rect16 rc = Rect(top);
    if (icon) {
        render_icon_align(Rect16(rc.TopLeft(), size_ui16_t({ icon->w, icon->h })), icon, GuiDefaults::ColorBack, Align_t::LeftTop());
    }
    rc += Rect16::Left_t(text_pos_after_icon);
    rc -= Rect16::Width_t(text_pos_after_icon);
    render(rc);
}

static constexpr size_t status_icon_width = WizardDefaults::status_icon_w;

/**
 * @brief Construct a new Self Test View Text With Icon And Result:: Self Test View Text With Icon And Result object
 *
 * if multiln == is_multiline::no, width has no effect
 * if multiln == is_multiline::yes && width == 0, full screen width is used (minus result icon width)
 * if multiln == is_multiline::yes && width != 0, width of result icon is subtracted from width
 *
 * @param txt       text
 * @param icon      icon
 * @param multiln   multiline flag
 * @param width     width to be able to measure height (number of lines) needed for multiline text
 */
SelfTestViewTextWithIconAndResult::SelfTestViewTextWithIconAndResult(string_view_utf8 txt, const img::Resource *icon, TestResult result, is_multiline multiln, Rect16::Width_t width)
    : SelfTestViewTextWithIcon(txt, icon, multiln, width == 0 ? Rect16::Width_t(GuiDefaults::ScreenWidth) : Rect16::Width_t(width - status_icon_width))
    , icon_result(ResultToIconId(result)) {}

void SelfTestViewTextWithIconAndResult::SetState(TestResult res) {
    icon_result = ResultToIconId(res);
}

void SelfTestViewTextWithIconAndResult::Draw(Rect16::Top_t top) const {
    Rect16 rc = Rect(top);
    if (icon) {
        render_icon_align(Rect16(rc.TopLeft(), size_ui16_t({ icon->w, icon->h })), icon, GuiDefaults::ColorBack, Align_t::LeftTop());
    }
    rc += Rect16::Left_t(text_pos_after_icon);
    rc -= Rect16::Width_t(text_pos_after_icon + status_icon_width);
    render(rc);
    render_icon_align(Rect16(WizardDefaults::status_icon_X_pos, rc.Top(), icon_result->w, icon_result->h), icon_result, GuiDefaults::ColorBack, Align_t::LeftTop());
}

const img::Resource *SelfTestViewTextWithIconAndResult::ResultToIconId(TestResult res) {
    switch (res) {
    case TestResult_Passed:
        return &img::ok_color_18x18;
    case TestResult_Failed:
        return &img::nok_color_18x18;
    default:
        break;
    }
    return &img::dash_18x18;
}
