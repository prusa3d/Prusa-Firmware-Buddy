// wizard_config.hpp
#pragma once
#include "selftest_sub_state.hpp"
#include "eeprom.h" // SelftestResult_Passed, SelftestResult_Failed
#include "GuiDefaults.hpp"

namespace WizardDefaults {
static constexpr size_t row_0 = 40;
static constexpr size_t txt_h = 22;
static constexpr size_t row_h = 24;
static constexpr size_t row_1 = row_0 + row_h;
static constexpr size_t MarginLeft = GuiDefaults::ScreenWidth <= 240 ? 6 : 24;
static constexpr size_t MarginRight = MarginLeft;
static constexpr size_t col_0 = MarginLeft;
static constexpr size_t col_after_icon = col_0 + 16 + 4; //icon has 16px
static constexpr size_t X_space = GuiDefaults::ScreenWidth - (MarginLeft + MarginRight);
static constexpr size_t Y_space = GuiDefaults::ScreenHeight - row_0;
static constexpr size_t status_icon_w = 20;
static constexpr size_t status_text_w = X_space - status_icon_w;
static constexpr size_t status_icon_X_pos = status_text_w + MarginLeft;
static constexpr size_t separator_width = 1;
static constexpr size_t separator_padding_bottom = 3;
static constexpr size_t progress_h = 3;
static constexpr size_t progress_row_h = progress_h + 8;
static constexpr size_t progress_LR_extra_margin = GuiDefaults::ScreenWidth <= 240 ? 4 : 0;
static constexpr size_t progress_LR_margin = progress_LR_extra_margin + MarginLeft;
static constexpr size_t progress_width = X_space - 2 * progress_LR_extra_margin;
static constexpr Rect16 RectSelftestFrame = GuiDefaults::RectScreenNoHeader; // must have full width because of footer
static constexpr Rect16 RectSelftestName = Rect16(MarginLeft, row_0, X_space, txt_h);
static constexpr Rect16 RectRadioButton(size_t lines_of_footer) {
    return GuiDefaults::GetButtonRect(RectSelftestFrame) - (lines_of_footer == 0 ? Rect16::Top_t(0) : Rect16::Top_t(GuiDefaults::FooterItemHeight * lines_of_footer + (lines_of_footer - 1) * GuiDefaults::FooterLinesSpace + GuiDefaults::FooterPadding.top + GuiDefaults::FooterPadding.bottom));
}
};
