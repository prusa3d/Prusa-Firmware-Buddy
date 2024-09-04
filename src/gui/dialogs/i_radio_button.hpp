/**
 * @file i_radio_button.hpp
 * @brief abstract parent for radio buttons
 */

#pragma once
#include <array>
#include "client_response.hpp"
#include "client_response_texts.hpp"
#include "window.hpp"
#include "../../lang/string_view_utf8.hpp"

using PhaseTexts = std::array<const char *, MAX_RESPONSES>;

class IRadioButton : public window_t {
public:
    // if greater than 0, we're drawing a fixed amount of buttons
    // used for MMU where we want to draw 3 buttons corresponding to the physical MMU buttons
    size_t fixed_width_buttons_count { 0 };
    static constexpr size_t max_buttons = 4;
    using Responses_t = std::array<Response, max_buttons>; // maximum is 4 responses (4B), better to pass by value
private:
    bool disabled_drawing_selected { false }; ///< used for when radio button is not the only scrollable window on the screen to allow no button drawn

    void draw_0_btn();
    void draw_1_btn();
    /// btn_count cannot exceed MAX_DIALOG_BUTTON_COUNT
    void draw_n_btns(size_t btn_count);

    struct Layout {
        PhaseTexts txts_to_print;
        Rect16 splits[GuiDefaults::MAX_DIALOG_BUTTON_COUNT];
        Rect16 spaces[GuiDefaults::MAX_DIALOG_BUTTON_COUNT - 1];
        uint8_t text_widths[GuiDefaults::MAX_DIALOG_BUTTON_COUNT];
    };
    Layout getNormalBtnRects(size_t count) const;

public:
    /**
     * @brief Construct a new Radio Button object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     * @param count  button count
     */
    IRadioButton(window_t *parent, Rect16 rect, size_t count);

    // No postfix increment/decrement operator, it would have to return button by value.
    // it would not be a problem, but buttons are not meant to be used that way
    IRadioButton &operator++(); // Prefix increment operator no overflow
    IRadioButton &operator--(); // Prefix decrement operator no underflow

    Response Click() const; // click returns response to be send, 0 buttons will return Response::_none
    bool IsEnabled(size_t index) const;

    void SetBtnIndex(uint8_t index);
    void SetBtn(Response btn);
    uint8_t GetBtnIndex() const { return flags.class_specific.button_index; }
    virtual std::optional<size_t> IndexFromResponse(Response btn) const = 0;

    void SetBtnCount(uint8_t cnt) { flags.class_specific.button_count = cnt & ((1 << RESPONSE_BITS) - 1); }
    uint8_t GetBtnCount() const { return flags.class_specific.button_count; }

    // Disables automatic redrawing of the currently selected button (useful when radio_button is not the only scrollable window on the screen)
    void DisableDrawingSelected();
    // Enables automatic redrawing of the currently selected button (useful when radio_button is not the only scrollable window on the screen)
    void EnableDrawingSelected();

    void set_fixed_width_buttons_count(size_t count) {
        fixed_width_buttons_count = count;
    }

    Rect16 get_rect_for_touch() const override;

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
    virtual void screenEvent(window_t *sender, GUI_event_t event, void *const param) override;

    virtual void unconditionalDraw() override;
    virtual Response responseFromIndex(size_t index) const = 0;
    virtual const PhaseTexts *getAlternativeTexts() const { return nullptr; }

    void invalidateWhatIsNeeded();
    void validateBtnIndex(); // needed for iconned layout
    bool isIndexValid(size_t index);
    size_t maxSize() const; // depends id it is iconned

    // TODO: REMOVEME BFW-6028
    static Responses_t generateResponses(const PhaseResponses &resp);

    static size_t cnt_labels(const PhaseTexts *labels);
    static size_t cnt_responses(Responses_t resp);
    static size_t cnt_buttons(const PhaseTexts *labels, Responses_t resp);

    // radio buttons currently do not support layout change
    // it is done by having multiple radio buttons and show/hide them
    virtual void set_layout(ColorLayout) override {}
};
