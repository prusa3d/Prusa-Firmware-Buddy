#include "DialogRadioButton.hpp"
#include <algorithm> //find
#include "button_draw.h"
#include "sound_C_wrapper.h"
/*****************************************************************************/
//static variables and methods
static const PhaseResponses no_responses = { Response::_none, Response::_none, Response::_none, Response::_none }; //used in constructor

size_t RadioButton::cnt_labels(const PhaseTexts &labels) {
    return (std::find_if(labels.begin(), labels.end(), [](const char *s) { return s[0] == '\0'; })) - labels.begin();
}

size_t RadioButton::cnt_responses(const PhaseResponses &resp) {
    return (std::find(resp.begin(), resp.end(), Response::_none)) - resp.begin();
}

size_t RadioButton::cnt_buttons(const PhaseTexts &labels, const PhaseResponses &resp) {
    size_t lbls = cnt_labels(labels);
    size_t cmds = cnt_responses(resp);
    return lbls > cmds ? lbls : cmds;
}
/*****************************************************************************/
//nonstatic variables and methods
RadioButton::RadioButton(const Window &window, const PhaseResponses &resp, const PhaseTexts &labels)
    : win(window)
    , responses(resp)
    , texts(labels)
    , btn_count(cnt_buttons(labels, resp))
    , selected_index(0)
    , need_redraw(true) {
}

RadioButton::RadioButton(const Window &window, const PhaseTexts &labels)
    : RadioButton(window, no_responses, labels) {
}

//no overflow
RadioButton &RadioButton::operator++() {
    if ((selected_index + 1) < btn_count) {
        ++selected_index; //btn_count can be 0
        need_redraw = true;
    } else {
        Sound_Play(eSOUND_TYPE_BlindAlert);
    }
    return *this;
}

//no underflow
RadioButton &RadioButton::operator--() {
    if (selected_index > 0) {
        --selected_index;
        need_redraw = true;
    } else {
        Sound_Play(eSOUND_TYPE_BlindAlert);
    }
    return *this;
}

bool RadioButton::Draw() {
    if (!need_redraw)
        return false;
    DrawForced();
    return true;
}

void RadioButton::DrawForced() {
    switch (btn_count) {
    case 0:
        draw_0_btn(); //cannot use draw_n_btns, would div by 0
        break;
    case 1:
        draw_1_btn(); //could use draw_n_btns, but this is much faster
        break;
    default:
        draw_n_btns(btn_count);
        break;
    }

    need_redraw = false;
}

Response RadioButton::Click() const {
    return responses[selected_index];
}

void RadioButton::draw_0_btn() const {
    display::FillRect(win.rect, win.color_back);
}

void RadioButton::draw_1_btn() const {
    button_draw(win.rect, texts[0], win.pfont, IsEnabled());
}

void RadioButton::draw_n_btns(size_t btn_count) const {
    rect_ui16_t rc_btn = win.rect;
    int16_t btn_width = rc_btn.w / btn_count - gui_defaults.btn_spacing * (btn_count - 1);
    rc_btn.w = btn_width;

    for (size_t i = 0; i < btn_count; ++i) {
        button_draw(rc_btn, texts[i], win.pfont, selected_index == i && IsEnabled());

        if (i + 1 < btn_count) {
            //space between buttons
            rc_btn.x += btn_width;
            rc_btn.w = gui_defaults.btn_spacing;
            display::FillRect(rc_btn, win.color_back);

            //nextbutton coords
            rc_btn.x += gui_defaults.btn_spacing;
            rc_btn.w = btn_width + gui_defaults.btn_spacing;
        }
    }
    rc_btn.x += rc_btn.w; //start of black space after button (if exists)
    int black_space_w = int(win.rect.x + win.rect.w) - int(rc_btn.x);
    if (black_space_w > 0) {
        rc_btn.w = black_space_w;
        display::FillRect(rc_btn, win.color_back);
    }
}

bool RadioButton::IsEnabled() const {
    return responses[0] != Response::_none; //faster than cnt_responses(responses)!=0
}
