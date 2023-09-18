/**
 * @file wizard_config.hpp
 */
#pragma once
#include "GuiDefaults.hpp"

namespace WizardDefaults {
#ifdef USE_ST7789
inline constexpr size_t row_0 = GuiDefaults::HeaderHeight;
inline constexpr size_t txt_h = 18;
inline constexpr size_t MarginLeft = 6;
inline constexpr size_t progress_LR_extra_margin = 4;
#else
inline constexpr size_t row_0 = 40;
inline constexpr size_t txt_h = 22;
inline constexpr size_t MarginLeft = 24;
inline constexpr size_t progress_LR_extra_margin = 0;
#endif
inline constexpr size_t row_h = txt_h + 2;
inline constexpr size_t row_1 = row_0 + row_h;
inline constexpr size_t MarginRight = MarginLeft;
inline constexpr size_t col_0 = MarginLeft;
inline constexpr size_t col_after_icon = col_0 + 16 + 4; // icon has 16px
inline constexpr size_t X_space = GuiDefaults::ScreenWidth - (MarginLeft + MarginRight);
inline constexpr size_t Y_space = GuiDefaults::ScreenHeight - row_0;
inline constexpr size_t status_icon_w = 20;
inline constexpr size_t status_text_w = X_space - status_icon_w;
inline constexpr size_t status_icon_X_pos = status_text_w + MarginLeft;
inline constexpr size_t separator_width = 1;
inline constexpr size_t separator_padding_bottom = 3;
inline constexpr size_t progress_h = 3;
inline constexpr size_t progress_row_h = progress_h + 8;
inline constexpr size_t progress_LR_margin = progress_LR_extra_margin + MarginLeft;
inline constexpr size_t progress_width = X_space - 2 * progress_LR_extra_margin;
inline constexpr Rect16 RectSelftestFrame = GuiDefaults::RectScreenNoHeader; // must have full width because of footer
inline constexpr Rect16 RectSelftestName = Rect16(MarginLeft, row_0, X_space, txt_h);
static constexpr Rect16 RectRadioButton(size_t lines_of_footer) {
    return GuiDefaults::GetButtonRect(RectSelftestFrame) - (lines_of_footer == 0 ? Rect16::Top_t(0) : Rect16::Top_t(GuiDefaults::FooterItemHeight * lines_of_footer + (lines_of_footer - 1) * GuiDefaults::FooterLinesSpace + GuiDefaults::FooterPadding.top + GuiDefaults::FooterPadding.bottom));
}
}; // namespace WizardDefaults
