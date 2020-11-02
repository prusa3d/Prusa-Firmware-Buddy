#pragma once
#include <array>
#include "client_response.hpp"
#include "dialog_response.hpp"
#include "window.hpp"

//created from array of responses and array of labels
//responses are counted and stored into btn_count
//if there is more labels than buttons, "additional buttons" are not acessible
//if there is less labels than buttons, "remaining buttons" have no labels
class RadioButton : public AddSuperWindow<window_t> {
    font_t *pfont;
    const PhaseResponses *responses;
    const PhaseTexts *texts;

    void SetBtnCount(uint8_t cnt) { flags.mem_array_u08[0] = cnt & (RESPONSE_BITS + 1); }
    const uint8_t GetBtnCount() const { return flags.mem_array_u08[0]; }

    static void button_draw(Rect16 rc_btn, string_view_utf8 text, const font_t *pf, bool is_selected);

    void draw_0_btn() const;
    void draw_1_btn() const;
    /// btn_count cannot exceed MAX_DIALOG_BUTTON_COUNT
    void draw_n_btns(const size_t btn_count) const;

    static size_t cnt_labels(const PhaseTexts *labels);
    static size_t cnt_responses(const PhaseResponses *resp);
    static size_t cnt_buttons(const PhaseTexts *labels, const PhaseResponses *resp);

public:
    RadioButton(window_t *parent, Rect16 rect, const PhaseResponses *resp, const PhaseTexts *labels); //has response == buttons enabled
    // No postfix increment/decrement operator, it would have to return button by value.
    // it would not be a problem, but buttons are not ment to be used that way
    RadioButton &operator++(); // Prefix increment operator no overflow
    RadioButton &operator--(); // Prefix decrement operator no underflow

    Response Click() const; //click returns response to be send, 0 buttons will return Response::_none
    bool IsEnabled() const;
    void Change(const PhaseResponses *responses, const PhaseTexts *texts);

    void SetBtnIndex(uint8_t index) { flags.mem_array_u08[1] = index < flags.mem_array_u08[0] ? index : 0; }
    uint8_t GetBtnIndex() const { return flags.mem_array_u08[1]; }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    virtual void unconditionalDraw() override;
};
