#pragma once
#include <array>
#include "dialog_commands.hpp"
#include "gui.h"

using PhaseTexts = std::array<const char *, MAX_COMMANDS>;

//created from array of commands and array of labels
//commands are counted and stored into btn_count
//if there is more labels than buttons, "additional buttons" are not acessible
//if there is less labels than buttons, "remaining buttons" have no labels
class RadioButton {
    struct window_t {
        font_t *pfont;
        color_t color_back;
        rect_ui16_t rect;
    };
    const window_t &win;
    const PhaseCommands &commands;
    const PhaseTexts &texts;
    const uint8_t btn_count : COMMAND_BITS;
    uint8_t selected_index : COMMAND_BITS;
    bool need_redraw : 1;
    bool is_enabled : 1;

    rect_ui16_t get_button_size() const;
    void draw_0_btn() const;
    void draw_1_btn() const;
    void draw_n_btn() const;

public:
    RadioButton(const window_t window, const PhaseCommands cmmnds, const PhaseTexts labels, bool enabled = true);

    // No postfix increment/decrement operator, it would have to return button by value.
    // it would not be a problem, but buttons are not ment to be used that way
    RadioButton &operator++(); // Prefix increment operator no overflow
    RadioButton &operator--(); // Prefix decrement operator no underflow

    bool Draw();           //draw only when need_redraw, return if did
    void DrawForced();     //draw no matter need_redraw
    Command Click() const; //click returns command to be send, 0 buttons will return Command::_NONE
};
