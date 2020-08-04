#include "DialogStateful.hpp"
#include "DialogRadioButton.hpp"
#include "gui.hpp"
#include "../lang/i18n.h"

static constexpr uint8_t PROGRESS_BAR_X_PAD = 10;
static constexpr uint8_t PROGRESS_BAR_Y_PAD = 30;
static constexpr uint8_t PROGRESS_BAR_H = 16;
static constexpr uint8_t PROGRESS_BAR_TEXT_H = 30;

//*****************************************************************************
IDialogStateful::IDialogStateful(const char *name, int16_t WINDOW_CLS_)
    : IDialog(WINDOW_CLS_)
    , id_capture(window_capture())
    , color_text(gui_defaults.color_text)
    , font(gui_defaults.font)
    , font_title(gui_defaults.font_big)
    , padding(gui_defaults.padding)
    , flags(0)
    , phase(0)
    , progress(-1)
    , title(name) {
    window_popup_ptr = this;
    gui_reset_jogwheel();
    gui_invalidate();
    SetCapture();
}

bool IDialogStateful::Change(uint8_t phs, uint8_t progress_tot, uint8_t /*progr*/) {
    if (!can_change(phs))
        return false;
    if (phase != phs) {
        phaseExit();
        phase = phs;
        flags |= DLG_PHA_CH;
        phaseEnter();
    }
    if (progress_tot != progress) {
        progress = progress_tot;
        flags |= DLG_PRO_CH;
    }
    gui_invalidate();
    return true;
}

IDialogStateful::~IDialogStateful() {
    window_destroy(id);
    if (window_ptr(id_capture))
        window_ptr(id_capture)->SetCapture();
    window_t *pWin = window_ptr(0);
    if (pWin != 0)
        pWin->Invalidate();
}

void IDialogStateful::draw_frame() {
    const uint16_t w = display::GetW() - 1 - rect.x + 1;  /// last - first + 1
    const uint16_t h = display::GetH() - 67 - rect.y + 1; /// last - first + 1
    display::DrawRect(rect_ui16(rect.x, rect.y, w, h), COLOR_GRAY);
}

//todo this should be moved elsewhere
void progress_draw(rect_ui16_t win_rect, const font_t *font, color_t color_back,
    color_t color_text, padding_ui8_t padding, uint8_t progress) {

    const uint16_t progress_w = win_rect.w - 2 * PROGRESS_BAR_X_PAD;
    const uint16_t done_w = (progress_w * progress) / 100;

    const rect_ui16_t rc_done = rect_ui16(
        win_rect.x + PROGRESS_BAR_X_PAD,
        win_rect.y + PROGRESS_BAR_Y_PAD,
        done_w,
        PROGRESS_BAR_H);
    display::FillRect(rc_done, COLOR_ORANGE);

    const rect_ui16_t rc_todo = rect_ui16(
        rc_done.x + done_w,
        rc_done.y,
        progress_w - rc_done.w,
        PROGRESS_BAR_H);
    display::FillRect(rc_todo, COLOR_GRAY);

    const rect_ui16_t rc_text = rect_ui16(rc_done.x, rc_done.y + PROGRESS_BAR_H, progress_w, PROGRESS_BAR_TEXT_H);
    char text[6];
    snprintf(text, sizeof(text), "%d%%", progress);
    // this MakeRAM is safe - text is not necessary after render_text_align finishes its work
    render_text_align(rc_text, string_view_utf8::MakeRAM((const uint8_t *)text), font, color_back, color_text, padding, ALIGN_CENTER);
}

//todo this should be moved elsewhere
void progress_clr(rect_ui16_t win_rect, const font_t * /*font*/, color_t color_back) {

    const rect_ui16_t rc = rect_ui16(
        win_rect.x + PROGRESS_BAR_X_PAD,
        win_rect.y + PROGRESS_BAR_Y_PAD,
        win_rect.w - 2 * PROGRESS_BAR_X_PAD,
        PROGRESS_BAR_H + PROGRESS_BAR_TEXT_H);

    display::FillRect(rc, color_back);
}

void IDialogStateful::draw_progress() {
    if (progress <= 100) {
        if (flags & DLG_PRO_CH)
            progress_draw(rect, font_title, color_back, color_text, padding, progress);
    } else {
        //do not draw progress at all
        progress_clr(rect, font_title, color_back);
    }
}

void IDialogStateful::draw_phase_text(string_view_utf8 text) {
    rect_ui16_t rc_sta = rect;
    size_t nl = 0; //number of new lines

    //count '\n' in nl, search by moving start (s)
    unichar c;
    while ((c = text.getUtf8Char()) != 0) {
        if (c == '\n')
            ++nl;
    }
    rc_sta.h = 30 + font_title->h * nl;
    rc_sta.y += (30 + 46);
    rc_sta.x += 2;
    rc_sta.w -= 4;

    //todo prerelease hack todo text window just should be CENTER_TOP aligned and bigger
    //erase previous text
    rect_ui16_t rc = rc_sta;
    rc.h = get_radio_button_size().y - rc_sta.y;
    display::FillRect(rc, color_back);

    text.rewind();
    render_text_align(rc_sta, text, font_title,
        color_back, color_text, padding, ALIGN_CENTER);
}
