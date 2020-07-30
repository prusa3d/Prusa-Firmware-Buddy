#include "DialogStateful.hpp"
#include "guitypes.h"
#include "../lang/i18n.h"
#include "resource.h" //IDR_FNT_BIG

static constexpr uint8_t PROGRESS_BAR_X_PAD = 10;
static constexpr uint8_t PROGRESS_BAR_Y_PAD = 30;
static constexpr uint8_t PROGRESS_BAR_H = 16;
static constexpr uint8_t PROGRESS_BAR_TEXT_H = 30;

rect_ui16_t get_title_size(rect_ui16_t rect) {
    rect.h = 30;
    return rect;
}

rect_ui16_t get_progress_size(rect_ui16_t rect) {
    rect.x += PROGRESS_BAR_X_PAD;
    rect.y += PROGRESS_BAR_Y_PAD;
    rect.w -= 2 * PROGRESS_BAR_X_PAD;
    rect.h = PROGRESS_BAR_H + PROGRESS_BAR_TEXT_H;
    return rect;
}

rect_ui16_t get_label_size(rect_ui16_t rect) {
    rect.y += (30 + 46);
    rect.x += 2;
    rect.w -= 4;
    rect.h = 60;
    return rect;
}

//*****************************************************************************
IDialogStateful::IDialogStateful(string_view_utf8 name)
    : IDialog()
    , title(this, get_title_size(rect), is_closed_on_click_t::no, name)
    , progress(this, get_progress_size(rect), PROGRESS_BAR_H, COLOR_ORANGE, COLOR_GRAY)
    , label(this, get_label_size(rect))
    , radio(this, get_radio_button_size(rect), nullptr, nullptr)
    , phase(0) {
    title.font = gui_defaults.font_big;
    title.SetAlignment(ALIGN_CENTER);
    progress.SetFont(resource_font(IDR_FNT_BIG));
    label.font = gui_defaults.font_big;
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
