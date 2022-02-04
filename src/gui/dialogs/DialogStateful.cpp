#include "DialogStateful.hpp"
#include "guitypes.hpp"
#include "i18n.h"
#include "resource.h" //IDR_FNT_BIG
#include "fsm_progress_type.hpp"

//suppress warning, gcc bug 80635
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
IDialogMarlin::IDialogMarlin(std::optional<Rect16> rc)
    : IDialog(rc ? (*rc) : GuiDefaults::RectDefaultDialog) {}
#pragma GCC diagnostic pop

static const constexpr int PROGRESS_BAR_X_PAD = 10;
static const constexpr int PROGRESS_BAR_Y_PAD = 30;
static const constexpr int PROGRESS_BAR_H = 16;
static const constexpr int PROGRESS_BAR_TEXT_H = 30;
static const constexpr int PROGRESS_BAR_TOP = PROGRESS_BAR_Y_PAD;
static const constexpr int PROGRESS_H = PROGRESS_BAR_H + PROGRESS_BAR_TEXT_H;

static const constexpr int LABEL_TEXT_PAD = 2;
static const constexpr int LABEL_TOP = PROGRESS_BAR_TOP + PROGRESS_H;

static const constexpr int RADIO_BUTTON_H = GuiDefaults::ButtonHeight + GuiDefaults::FrameWidth;

Rect16 get_title_rect(Rect16 rect) {
    return Rect16(rect.Left(), rect.Top(), rect.Width(), 30);
}

Rect16 get_progress_rect(Rect16 rect) {
    return Rect16(rect.Left() + PROGRESS_BAR_X_PAD, rect.Top() + PROGRESS_BAR_TOP, rect.Width() - 2 * PROGRESS_BAR_X_PAD, PROGRESS_H);
}

Rect16 get_label_rect(Rect16 rect) {
    const int RADION_BUTTON_TOP = rect.Height() - RADIO_BUTTON_H;
    const int LABEL_H = RADION_BUTTON_TOP - LABEL_TOP;
    return Rect16(rect.Left() + LABEL_TEXT_PAD, rect.Top() + LABEL_TOP, rect.Width() - 2 * LABEL_TEXT_PAD, LABEL_H);
}

//*****************************************************************************
IDialogStateful::IDialogStateful(string_view_utf8 name, std::optional<has_footer> child_has_footer)
    : IDialogMarlin(GuiDefaults::GetDialogRect(child_has_footer))
    , title(this, get_title_rect(GetRect()), is_multiline::no, is_closed_on_click_t::no, name)
    , progress(this, get_progress_rect(GetRect()), PROGRESS_BAR_H, COLOR_ORANGE, COLOR_GRAY)
    , label(this, get_label_rect(GetRect()), is_multiline::yes)
    , radio(this, (child_has_footer == has_footer::yes) ? GuiDefaults::GetButtonRect_AvoidFooter(GetRect()) : GuiDefaults::GetButtonRect(GetRect()), nullptr, nullptr)
    , phase(0) {
    title.font = GuiDefaults::FontBig;
    title.SetAlignment(Align_t::Center());
    progress.SetFont(resource_font(IDR_FNT_BIG));
    label.font = GuiDefaults::FontBig;
    label.SetAlignment(Align_t::CenterTop());
}

bool IDialogStateful::change(uint8_t phs, fsm::PhaseData data) {
    if (!can_change(phs))
        return false;
    if (phase != phs) {
        phaseExit();
        phase = phs;
        phaseEnter();
    }

    ProgressSerializer serializer(data);
    progress.SetValue(serializer.progress);
    //Invalidate();
    return true;
}
