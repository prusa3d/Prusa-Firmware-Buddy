#include "DialogRadioButton.hpp"
#include <algorithm> //find
#include "button_draw.h"

RadioButton::RadioButton(const window_t window, const PhaseCommands cmmnds, const PhaseTexts labels, bool enabled)
    : win(window)
    , commands(cmmnds)
    , texts(labels)
    , btn_count((std::find(commands.begin(), commands.end(), Command::_NONE)) - commands.begin())
    , selected_index(0)
    , need_redraw(true)
    , is_enabled(enabled) {
}

//no overflow
RadioButton &RadioButton::operator++() {
    if ((selected_index + 1) < btn_count)
        ++selected_index; //btn_count can be 0
    return *this;
}

//no underflow
RadioButton &RadioButton::operator--() {
    if (selected_index > 0)
        --selected_index;
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
        draw_0_btn(); //cannot use draw_n_btn, would div by 0
        break;
    case 1:
        draw_1_btn(); //could use draw_n_btn, but this is much faster
        break;
    default:
        draw_n_btn();
        break;
    }

    need_redraw = false;
}

Command RadioButton::Click() const {
    return commands[selected_index];
}

rect_ui16_t RadioButton::get_button_size() const {
    rect_ui16_t rc_btn = win.rect;
    rc_btn.y += (rc_btn.h - 40); // 30pixels for button (+ 10 space for grey frame)
    rc_btn.h = 30;
    rc_btn.x += 6;
    rc_btn.w -= 12;
    return rc_btn;
}

void RadioButton::draw_0_btn() const {
    display->fill_rect(win.rect, win.color_back);
}

void RadioButton::draw_1_btn() const {
    rect_ui16_t rc_btn = get_button_size();
    button_draw(rc_btn, texts[0], win.pfont, is_enabled);
}

void RadioButton::draw_n_btn() const {
    rect_ui16_t rc_btn = win.rect;

    int16_t btn_width = rc_btn.w / 2 - gui_defaults.btn_spacing;

    rc_btn.w = btn_width;
    //lhs button
    button_draw(rc_btn, texts[0], win.pfont, selected_index == 0 && is_enabled);

    //more difficult calculations of coords to avoid round errors

    //space between buttons
    rc_btn.x += btn_width;
    rc_btn.w = get_button_size().w - rc_btn.w * 2;
    display->fill_rect(rc_btn, win.color_back);

    //distance of both buttons from screen sides is same
    rc_btn.x += rc_btn.w;
    rc_btn.w = btn_width;

    //rhs button
    button_draw(rc_btn, texts[1], win.pfont, selected_index == 1 && is_enabled);
}
