/**
 * @file radio_button_fsm.hpp
 */

#pragma once
#include "radio_button.hpp"
#include "marlin_client.hpp"

/// A sensible implementation of RadioButtonFsm.
/// Ignore the other one
class RadioButtonFSM : public RadioButton {

public:
    RadioButtonFSM(window_t *parent, Rect16 rect, FSMAndPhase fsm_phase);

    inline FSMAndPhase fsm_and_phase() const {
        return fsm_and_phase_;
    }

    void set_fsm_and_phase(FSMAndPhase target);

    // TODO: Removeme
    // Ugly, for backwards compatibility reasons
    inline void Change(FSMAndPhase target) {
        set_fsm_and_phase(target);
    }

protected:
    void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    FSMAndPhase fsm_and_phase_;
};

/**
 * @brief radio button bound to fsm
 * unlike normal radio button it does not store responses but fsm phase
 * responses are generated from it at run time
 * this behavior allows to handle click automatically
 */
/// !!! DEPRECATED, USE RadioButtonFSM
template <class FSM_PHASE>
class RadioButtonFsm : public IRadioButton {

    size_t cnt_buttons(FSM_PHASE phase) const {
        const PhaseResponses &resp = ClientResponses::GetResponses(phase); // ClientResponses::GetResponses returns array of 16 responses
        return std::min(cnt_responses(generateResponses(resp)), max_buttons); // generateResponses cuts it to 4
    }

public:
    /**
     * @brief Construct a new Radio Button object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     * @param phase  fsm phase
     * @param labels array of button labels, if is set to nullptr, strings are assigned as default ones from given responses
     */
    RadioButtonFsm(window_t *parent, Rect16 rect, FSM_PHASE phase)
        : IRadioButton(parent, rect, cnt_buttons(phase))
        , current_phase(phase) {}

    void Change(FSM_PHASE phase) {
        if (current_phase == phase) {
            return;
        }
        current_phase = phase;
        SetBtnCount(fixed_width_buttons_count > 0 ? fixed_width_buttons_count : cnt_buttons(phase));

        // in iconned layout index will stay
        if (fixed_width_buttons_count == 0) {
            SetBtnIndex(0);
        }

        validateBtnIndex();

        invalidateWhatIsNeeded();
    }

    virtual std::optional<size_t> IndexFromResponse(Response btn) const override {
        uint8_t index = ClientResponses::GetIndex(current_phase, btn);
        if (index < maxSize()) {
            return index;
        }

        return std::nullopt;
    }

protected:
    FSM_PHASE current_phase;

    virtual Response responseFromIndex(size_t index) const override {
        return ClientResponses::GetResponse(current_phase, index);
    }

    void windowEvent(window_t *sender, GUI_event_t event, void *param) override {
        switch (event) {
        case GUI_event_t::CLICK: {
            Response response = Click();
            marlin_client::FSM_response(current_phase, response);
            break;
        }
        default:
            IRadioButton::windowEvent(sender, event, param);
        }
    }
};
