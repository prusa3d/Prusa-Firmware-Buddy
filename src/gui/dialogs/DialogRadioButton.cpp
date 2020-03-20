#include "DialogRadioButton.hpp"
#include <algorithm> //find
#include "button_draw.h"
/*****************************************************************************/
//static variables and methods
static const PhaseCommands disabled_commands = { Command::_NONE, Command::_NONE, Command::_NONE, Command::_NONE }; //used in constructor

size_t RadioButton::cnt_labels(const PhaseTexts &labels) {
    return (std::find_if(labels.begin(), labels.end(), [](const char *s) { return strcmp(s, ""); })) - labels.begin();
}

size_t RadioButton::cnt_commands(const PhaseCommands &cmmnds) {
    return (std::find(cmmnds.begin(), cmmnds.end(), Command::_NONE)) - cmmnds.begin();
}

size_t RadioButton::cnt_buttons(const PhaseTexts &labels, const PhaseCommands &cmmnds) {
    size_t lbls = cnt_labels(labels);
    size_t cmds = cnt_commands(cmmnds);
    return lbls > cmds ? lbls : cmds;
}
/*****************************************************************************/
//nonstatic variables and methods
RadioButton::RadioButton(const Window &window, const PhaseCommands &cmmnds, const PhaseTexts &labels)
    : win(window)
    , commands(cmmnds)
    , texts(labels)
    , btn_count(cnt_buttons(labels, cmmnds))
    , selected_index(0)
    , need_redraw(true) {
}

RadioButton::RadioButton(const Window &window, const PhaseTexts &labels)
    : RadioButton(window, disabled_commands, labels) {
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
        draw_n_btn(btn_count);
        break;
    }

    need_redraw = false;
}

Command RadioButton::Click() const {
    return commands[selected_index];
}

void RadioButton::draw_0_btn() const {
    display->fill_rect(win.rect, win.color_back);
}

void RadioButton::draw_1_btn() const {
    button_draw(win.rect, texts[0], win.pfont, IsEnabled());
}

void RadioButton::draw_n_btn(size_t btn_count) const {
    rect_ui16_t rc_btn = win.rect;

    int16_t btn_width = rc_btn.w / btn_count - gui_defaults.btn_spacing * (btn_count - 1);

    rc_btn.w = btn_width;
    //lhs button
    for (size_t i = 0; i < btn_count; ++i) {
        button_draw(rc_btn, texts[i], win.pfont, selected_index == i && IsEnabled());

        if (i + 1 < btn_count) {
            //space between buttons
            rc_btn.x += btn_width;
            rc_btn.w = gui_defaults.btn_spacing;
            display->fill_rect(rc_btn, win.color_back);

            //nextbutton coords
            rc_btn.x += gui_defaults.btn_spacing;
            rc_btn.w = btn_width;
        }
    }
}

bool RadioButton::IsEnabled() const {
    return commands[0] != Command::_NONE; //faster than cnt_commands(cmmnds)!=0
}
