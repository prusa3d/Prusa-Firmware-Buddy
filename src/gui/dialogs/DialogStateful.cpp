#include "DialogStateful.hpp"
#include "guitypes.hpp"
#include "i18n.h"
#include "fonts.hpp" //IDR_FNT_BIG

// suppress warning, gcc bug 80635
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
IDialogMarlin::IDialogMarlin(std::optional<Rect16> rc)
    : IDialog(rc ? (*rc) : GuiDefaults::DialogFrameRect) {}
#pragma GCC diagnostic pop

static const constexpr int PROGRESS_BAR_H = 16;
static const constexpr int PROGRESS_BAR_TEXT_H = 30;
static const constexpr int PROGRESS_H = GuiDefaults::EnableDialogBigLayout ? 80 : (PROGRESS_BAR_H + PROGRESS_BAR_TEXT_H);
static const constexpr int LABEL_TEXT_PAD = 2;
static const constexpr int PROGRESS_BAR_CORNER_RADIUS = GuiDefaults::EnableDialogBigLayout ? 4 : 0;
static const constexpr int TITLE_HEIGHT = 30;
static const constexpr int RADIO_BUTTON_H = GuiDefaults::ButtonHeight + GuiDefaults::FramePadding;
static const constexpr int TITLE_TOP = 70;
static const constexpr int PROGRESS_TOP = GuiDefaults::EnableDialogBigLayout ? 100 : 30;
static const constexpr int LABEL_TOP = GuiDefaults::EnableDialogBigLayout ? 180 : PROGRESS_TOP + PROGRESS_H;
static const constexpr int PROGRESS_BAR_X_PAD = GuiDefaults::EnableDialogBigLayout ? 24 : 10;

Rect16 IDialogStateful::get_frame_rect(Rect16 rect, std::optional<has_footer> dialog_has_footer) {
    return Rect16(
        rect.Left(),
        rect.Top(),
        rect.Width(),
        rect.Height() - (!dialog_has_footer || *dialog_has_footer == has_footer::no ? 0 : GuiDefaults::FooterHeight));
}

Rect16 IDialogStateful::get_title_rect(Rect16 rect) {
    return Rect16(rect.Left(), GuiDefaults::EnableDialogBigLayout ? TITLE_TOP : (int)rect.Top(), rect.Width(), TITLE_HEIGHT);
}

Rect16 IDialogStateful::get_progress_rect(Rect16 rect) {
    return Rect16(rect.Left() + PROGRESS_BAR_X_PAD, GuiDefaults::EnableDialogBigLayout ? PROGRESS_TOP : rect.Top() + PROGRESS_TOP, rect.Width() - 2 * PROGRESS_BAR_X_PAD, PROGRESS_H);
}

Rect16 IDialogStateful::get_label_rect(Rect16 rect, std::optional<has_footer> dialog_has_footer) {

    const int RADION_BUTTON_TOP = !dialog_has_footer || *dialog_has_footer == has_footer::no ? rect.Height() - RADIO_BUTTON_H : rect.Height() - RADIO_BUTTON_H - GuiDefaults::FooterHeight;
    const int LABEL_H = RADION_BUTTON_TOP - LABEL_TOP;
    return Rect16(rect.Left() + LABEL_TEXT_PAD, GuiDefaults::EnableDialogBigLayout ? LABEL_TOP : rect.Top() + LABEL_TOP, rect.Width() - 2 * LABEL_TEXT_PAD, LABEL_H);
}

//*****************************************************************************
IDialogStateful::IDialogStateful(string_view_utf8 name, std::optional<has_footer> child_has_footer)
    : IDialogMarlin(GuiDefaults::GetDialogRect(child_has_footer))
    , progress_frame(this, get_frame_rect(GetRect(), child_has_footer))
    , title(&progress_frame, get_title_rect(GetRect()), is_multiline::no, is_closed_on_click_t::no, name)
    , progress(&progress_frame, get_progress_rect(GetRect()), PROGRESS_BAR_H, COLOR_ORANGE, GuiDefaults::EnableDialogBigLayout ? COLOR_DARK_GRAY : COLOR_GRAY, PROGRESS_BAR_CORNER_RADIUS)
    , label(&progress_frame, get_label_rect(GetRect(), child_has_footer), is_multiline::yes) {
    title.set_font(GuiDefaults::FontBig);
    title.SetAlignment(Align_t::Center());
    progress.set_font(resource_font(IDR_FNT_BIG));

    label.set_font(GuiDefaults::EnableDialogBigLayout ? resource_font(IDR_FNT_SPECIAL) : GuiDefaults::FontBig);
    label.SetAlignment(Align_t::CenterTop());
}
