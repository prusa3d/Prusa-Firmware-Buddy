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
public:
    struct Window {
        font_t *pfont;
        color_t color_back;
        rect_ui16_t rect;
    };

private:
    const Window &win;
    const PhaseCommands &commands;
    const PhaseTexts &texts;
    const uint8_t btn_count : COMMAND_BITS + 1;
    uint8_t selected_index : COMMAND_BITS;
    bool need_redraw : 1;

    void draw_0_btn() const;
    void draw_1_btn() const;
    void draw_n_btn(size_t btn_count) const;

    static size_t cnt_labels(const PhaseTexts &labels);
    static size_t cnt_commands(const PhaseCommands &cmmnds);
    static size_t cnt_buttons(const PhaseTexts &labels, const PhaseCommands &cmmnds);

public:
    RadioButton(const Window &window, const PhaseCommands &cmmnds, const PhaseTexts &labels); //has commands == buttons enabled
    RadioButton(const Window &window, const PhaseTexts &labels);                              //does not have commands == buttons disabled
    // No postfix increment/decrement operator, it would have to return button by value.
    // it would not be a problem, but buttons are not ment to be used that way
    RadioButton &operator++(); // Prefix increment operator no overflow
    RadioButton &operator--(); // Prefix decrement operator no underflow

    bool Draw();           //draw only when need_redraw, return if did
    void DrawForced();     //draw no matter need_redraw
    Command Click() const; //click returns command to be send, 0 buttons will return Command::_NONE
    bool IsEnabled() const;
};
