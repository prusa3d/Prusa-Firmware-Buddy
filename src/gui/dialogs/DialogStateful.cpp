#include "DialogStateful.hpp"
#include "guitypes.hpp"
#include "i18n.h"
#include "resource.h" //IDR_FNT_BIG

static constexpr uint8_t PROGRESS_BAR_X_PAD = 10;
static constexpr uint8_t PROGRESS_BAR_Y_PAD = 30;
static constexpr uint8_t PROGRESS_BAR_H = 16;
static constexpr uint8_t PROGRESS_BAR_TEXT_H = 30;

Rect16 get_title_size(Rect16 rect) {
    return Rect16(rect.Left(), rect.Top(), rect.Width(), 30);
}

Rect16 get_progress_size(Rect16 rect) {
    return Rect16(rect.Left() + PROGRESS_BAR_X_PAD, rect.Top() + PROGRESS_BAR_Y_PAD, rect.Width() - 2 * PROGRESS_BAR_X_PAD, PROGRESS_BAR_H + PROGRESS_BAR_TEXT_H);
}

Rect16 get_label_size(Rect16 rect) {
    return Rect16(rect.Left() + 2, rect.Top() + 30 + 46, rect.Width() - 4, 60);
}

//*****************************************************************************
IDialogStateful::IDialogStateful(string_view_utf8 name)
    : IDialog()
    , title(this, get_title_size(rect), is_closed_on_click_t::no, name)
    , progress(this, get_progress_size(rect), PROGRESS_BAR_H, COLOR_ORANGE, COLOR_GRAY)
    , label(this, get_label_size(rect))
    , radio(this, get_radio_button_size(rect), nullptr, nullptr)
    , phase(0) {
    title.font = GuiDefaults::FontBig;
    title.SetAlignment(ALIGN_CENTER);
    progress.SetFont(resource_font(IDR_FNT_BIG));
    label.font = GuiDefaults::FontBig;
    label.SetAlignment(ALIGN_CENTER);
}

bool IDialogStateful::Change(uint8_t phs, uint8_t progress_tot, uint8_t /*progr*/) {
    if (!can_change(phs))
        return false;
    if (phase != phs) {
        phaseExit();
        phase = phs;
        phaseEnter();
    }

    progress.SetValue(progress_tot <= 100 ? progress_tot : 0);
    //Invalidate();
    return true;
}
