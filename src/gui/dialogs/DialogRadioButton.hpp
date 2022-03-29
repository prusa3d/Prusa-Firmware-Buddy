#pragma once
#include <array>
#include "client_response.hpp"
#include "client_response_texts.hpp"
#include "window.hpp"
#include "../../lang/string_view_utf8.hpp"

/**
 * @brief RadioButton designed to be used id dialogs, it is bound to Response
 * created from array of responses and array of labels
 * responses are counted and stored into btn_count
 * if there is more labels than buttons, "additional buttons" are not acessible
 * if there is less labels than buttons, "remaining buttons" have no labels
 * has optional icons, in that case number of buttons is fixed to 3
 * in iconned mode can validate background
 */
class RadioButton : public AddSuperWindow<window_t> {
    static constexpr size_t max_icons = 3;
    static constexpr size_t max_buttons = 4;

public:
    using Responses_t = std::array<Response, max_buttons>; // maximum is 4 responses (4B), better to pass by value
private:
    font_t *pfont;
    Responses_t responses;
    const PhaseTexts *texts; // nullptr == autoset texts with default strings

    void SetBtnCount(uint8_t cnt) { flags.button_count = cnt & ((1 << RESPONSE_BITS) - 1); }
    const uint8_t GetBtnCount() const { return flags.button_count; }

    static void button_draw(Rect16 rc_btn, color_t back_color, color_t parent_color, string_view_utf8 text, const font_t *pf, bool is_selected);

    void draw_0_btn();
    void draw_1_btn();
    /// btn_count cannot exceed MAX_DIALOG_BUTTON_COUNT
    void draw_n_btns(const size_t btn_count);

    static size_t cnt_labels(const PhaseTexts *labels);
    static size_t cnt_responses(Responses_t resp);
    static size_t cnt_buttons(const PhaseTexts *labels, Responses_t resp);
    Rect16 getIconRect(uint8_t idx) const;
    Rect16 getLabelRect(uint8_t idx) const;

public:
    /**
     * @brief Construct a new Radio Button object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     */
    RadioButton(window_t *parent, Rect16 rect);
    /**
     * @brief Construct a new Radio Button object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     * @param resp   array of responses bound to buttons, has response == buttons enabled, only first four are used, rest is discarded
     * @param labels array of button labels, if is set to nullptr, strings are assigned as default ones from given responses
     */
    RadioButton(window_t *parent, Rect16 rect, const PhaseResponses &resp, const PhaseTexts *labels = nullptr);
    /**
     * @brief Construct a new Radio Button object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     * @param resp   array of responses bound to buttons, has response == buttons enabled
     * @param labels array of button labels, if is set to nullptr, strings are assigned as default ones from given responses
     */
    RadioButton(window_t *parent, Rect16 rect, Responses_t resp, const PhaseTexts *labels = nullptr);

    // No postfix increment/decrement operator, it would have to return button by value.
    // it would not be a problem, but buttons are not meant to be used that way
    RadioButton &operator++(); // Prefix increment operator no overflow
    RadioButton &operator--(); // Prefix decrement operator no underflow

    Response Click() const; //click returns response to be send, 0 buttons will return Response::_none
    bool IsEnabled() const;
    void Change(const PhaseResponses &resp, const PhaseTexts *txts = nullptr); // nullptr generates texts automatically, only first four responses are used, rest is discarded
    void Change(Responses_t resp, const PhaseTexts *txts = nullptr);           // nullptr generates texts automatically

    void SetBtnIndex(uint8_t index) { flags.button_index = (index < GetBtnCount()) ? index : 0; }
    uint8_t GetBtnIndex() const { return flags.button_index; }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    virtual void unconditionalDraw() override;
    void invalidateWhatIsNeeded();
    void validateBtnIndex(); //needed for iconned layout
    bool isIndexValid(size_t index);
    Response responseFromIndex(size_t index) const;
    size_t maxSize() const; //depends id it is iconned
    Responses_t generateResponses(const PhaseResponses &resp) const;
};
