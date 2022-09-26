/**
 * @file radio_button_fsm.hpp
 * @brief
 */

#pragma once
#include "radio_button.hpp"
#include "marlin_client.hpp"

/**
 * @brief radio button bound to fsm
 * unlike normal radio button it does not store responses but fsm phase
 * responses are generated from it at run time
 * this behavior allows to handle click automatically
 */
template <class FSM_PHASE>
class RadioButtonFsm : public AddSuperWindow<IRadioButton> {
    FSM_PHASE phase;

    size_t cnt_buttons(FSM_PHASE phs) const {
        const PhaseResponses &resp = ClientResponses::GetResponses(phs);      // ClientResponses::GetResponses returns array of 16 responses
        return std::min(cnt_responses(generateResponses(resp)), max_buttons); // generateResponses cuts it to 4
    }

public:
    /**
     * @brief Construct a new Radio Button object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     * @param phs    fsm phase
     * @param labels array of button labels, if is set to nullptr, strings are assigned as default ones from given responses
     */
    RadioButtonFsm(window_t *parent, Rect16 rect, FSM_PHASE phs)
        : AddSuperWindow<IRadioButton>(parent, rect, cnt_buttons(phs))
        , phase(phs) {}

    template <class FSM_Phase>
    void Change(FSM_Phase phs) {
        if (phase == phs)
            return;
        phase = phs;
        SetBtnCount(HasIcon() ? max_icons : cnt_buttons(phs));

        //in iconned layout index will stay
        if (!HasIcon()) {
            SetBtnIndex(0);
        }

        validateBtnIndex();

        invalidateWhatIsNeeded();
    }

    virtual std::optional<size_t> IndexFromResponse(Response btn) const override {
        uint8_t index = ClientResponses::GetIndex(phase, btn);
        if (index < maxSize())
            return index;

        return std::nullopt;
    }

protected:
    virtual Response responseFromIndex(size_t index) const override {
        return ClientResponses::GetResponse(phase, index);
    }

    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
        switch (event) {
        case GUI_event_t::CLICK: {
            Response response = Click();
            marlin_FSM_response(phase, response);
            break;
        }
        default:
            SuperWindowEvent(sender, event, param);
        }
    }
};
