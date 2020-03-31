#pragma once
#include <array>
#include "client_response.hpp"
#include "gui.h"

using PhaseTexts = std::array<const char *, MAX_RESPONSES>;

//created from array of responses and array of labels
//responses are counted and stored into btn_count
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
    const PhaseResponses &responses;
    const PhaseTexts &texts;
    const uint8_t btn_count : RESPONSE_BITS + 1;
    uint8_t selected_index : RESPONSE_BITS;
    bool need_redraw : 1;

    void draw_0_btn() const;
    void draw_1_btn() const;
    void draw_n_btns(size_t btn_count) const;

    static size_t cnt_labels(const PhaseTexts &labels);
    static size_t cnt_responses(const PhaseResponses &resp);
    static size_t cnt_buttons(const PhaseTexts &labels, const PhaseResponses &resp);

public:
    RadioButton(const Window &window, const PhaseResponses &cmmnds, const PhaseTexts &labels); //has response == buttons enabled
    RadioButton(const Window &window, const PhaseTexts &labels);                               //does not have response == buttons disabled
    // No postfix increment/decrement operator, it would have to return button by value.
    // it would not be a problem, but buttons are not ment to be used that way
    RadioButton &operator++(); // Prefix increment operator no overflow
    RadioButton &operator--(); // Prefix decrement operator no underflow

    bool Draw();            //draw only when need_redraw, return if did
    void DrawForced();      //draw no matter need_redraw
    Response Click() const; //click returns response to be send, 0 buttons will return Response::_none
    bool IsEnabled() const;
};
