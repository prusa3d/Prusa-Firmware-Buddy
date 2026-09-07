/// @file
#pragma once

#include <array>
#include <client_response.hpp>
#include <window.hpp>
#include <inplace_function.hpp>

class IRadioButton : public window_t {
public:
    // if greater than 0, we're drawing a fixed amount of buttons
    // used for MMU where we want to draw 3 buttons corresponding to the physical MMU buttons
    size_t fixed_width_buttons_count { 0 };
    static constexpr size_t max_buttons = 4;

    using Responses_t = std::array<Response, max_buttons>; // maximum is 4 responses (4B), better to pass by value

    using ClickCallback = stdext::inplace_function<void(Response)>;

public:
    /**
     * @brief Construct a new Radio Button object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     */
    IRadioButton(window_t *parent, Rect16 rect);

// TODO: REMOVEME completely BFW-6028
#if MAX_RESPONSES != 4
    /**
     * @brief Construct a new Radio Button object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     * @param resp   array of responses bound to buttons, has response == buttons enabled, only first four are used, rest is discarded
     */
    IRadioButton(window_t *parent, Rect16 rect, const PhaseResponses &resp);
#endif

    /**
     * @brief Construct a new Radio Button object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     * @param resp   array of responses bound to buttons, has response == buttons enabled
     */
    IRadioButton(window_t *parent, Rect16 rect, Responses_t resp);

    /// Constructs the radio button and binds it to the specified FSM
    /// Clicking will send the response to the FSM
    IRadioButton(window_t *parent, Rect16 rect, FSMAndPhase fsm_phase);

// TODO: REMOVEME completely BFW-6028
#if MAX_RESPONSES != 4
    void Change(const PhaseResponses &resp); // nullptr generates texts automatically, only first four responses are used, rest is discarded
#endif

    void Change(Responses_t resp); // nullptr generates texts automatically

    // TODO: Removeme
    // Ugly, for backwards compatibility reasons
    [[deprecated]] inline void Change(FSMAndPhase target) {
        set_fsm_and_phase(target);
    }

    /// Binds the button to a FSM - clicking will send a response to the FSM
    void set_fsm_and_phase(FSMAndPhase target);

    /// Binds the button to a FSM - clicking will send a response to the FSM
    void set_fsm_and_phase(FSMAndPhase target, PhaseResponses responses);

    void set_callback(const ClickCallback &set) {
        click_callback_ = set;
    }

private:
    bool disabled_drawing_selected { false }; ///< used for when radio button is not the only scrollable window on the screen to allow no button drawn

    void draw_0_btn();
    void draw_1_btn();
    /// btn_count cannot exceed MAX_DIALOG_BUTTON_COUNT
    void draw_n_btns(size_t btn_count);

    struct Layout {
        Rect16 splits[GuiDefaults::MAX_DIALOG_BUTTON_COUNT];
        Rect16 spaces[GuiDefaults::MAX_DIALOG_BUTTON_COUNT - 1];
        uint8_t text_widths[GuiDefaults::MAX_DIALOG_BUTTON_COUNT];
    };
    Layout getNormalBtnRects(size_t count) const;

public:
    Response Click() const; // click returns response to be send, 0 buttons will return Response::_none
    bool IsEnabled(size_t index) const;

    void SetBtnIndex(uint8_t index);

    void SetBtn(Response btn);
    uint8_t GetBtnIndex() const { return flags.class_specific.button_index; }
    std::optional<size_t> IndexFromResponse(Response btn) const;

    void SetBtnCount(uint8_t cnt) { flags.class_specific.button_count = std::min<uint8_t>(cnt, MAX_RESPONSES); }
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
    Response responseFromIndex(size_t index) const;

    void invalidateWhatIsNeeded();
    void validateBtnIndex(); // needed for iconned layout
    bool isIndexValid(int index);
    size_t maxSize() const; // depends id it is iconned

    // TODO: REMOVEME BFW-6028
    static Responses_t generateResponses(const PhaseResponses &resp);

    // radio buttons currently do not support layout change
    // it is done by having multiple radio buttons and show/hide them
    virtual void set_layout(ColorLayout) override {}

private:
    /// Does not clamp the index to the valid range.
    /// If the index is out of range, no button is selected.
    void set_button_index_nocheck(uint8_t index);

private:
    Responses_t responses {};

    /// Callback that is called when a button is pressed
    ClickCallback click_callback_;
};
