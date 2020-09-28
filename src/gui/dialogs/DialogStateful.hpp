#pragma once

#include "IDialog.hpp"
#include <array>
#include "DialogRadioButton.hpp"
#include "marlin_client.hpp"
#include "client_response.hpp"
#include "i18n.h"
#include "window_text.hpp"
#include "window_progress.hpp"

// function pointer for onEnter & onExit callbacks
using change_state_cb_t = void (*)();

class IDialogMarlin : public IDialog {
protected:
    virtual bool change(uint8_t phs, uint8_t progress_tot, uint8_t progress) = 0;

public:
    bool Change(uint8_t phs, uint8_t progress_tot, uint8_t progress) { return change(phs, progress_tot, progress); }
};

//abstract parent containing general code for any number of phases
class IDialogStateful : public IDialogMarlin {
public:
    struct State {
        State(const char *lbl, const PhaseResponses &btn_resp, const PhaseTexts &btn_labels, change_state_cb_t enter_cb = NULL, change_state_cb_t exit_cb = NULL)
            : label(lbl)
            , btn_resp(btn_resp)
            , btn_labels(btn_labels)
            , onEnter(enter_cb)
            , onExit(exit_cb) {}
        const char *label;
        const PhaseResponses &btn_resp;
        const PhaseTexts &btn_labels;
        // callbacks for phase start/end
        change_state_cb_t onEnter;
        change_state_cb_t onExit;
    };

protected:
    window_text_t title;
    window_progress_t progress;
    window_text_t label;
    RadioButton radio;
    uint8_t phase;

    virtual bool can_change(uint8_t phase) = 0;
    // must be virtual because of `states` list is in template protected
    virtual void phaseEnter() = 0;
    virtual void phaseExit() = 0;
    virtual bool change(uint8_t phs, uint8_t progress_tot, uint8_t progress) override;

public:
    IDialogStateful(string_view_utf8 name);
};

/*****************************************************************************/
//parent for stateful dialogs dialog
//use one of enumclass from "client_response.hpp" as T
template <class T>
class DialogStateful : public IDialogStateful {
public:
    enum { SZ = CountPhases<T>() };
    using States = std::array<State, SZ>;

protected:
    States states; //phase text and radiobutton + onEnter & onExit cb
public:
    DialogStateful(string_view_utf8 name, States st)
        : IDialogStateful(name)
        , states(st) {};

protected:
    virtual bool can_change(uint8_t phase) { return phase < SZ; }
    // get arguments callbacks and call them
    virtual void phaseEnter() {
        radio.Change(&states[phase].btn_resp, &states[phase].btn_labels);
        label.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)states[phase].label));
        if (states[phase].onEnter) {
            states[phase].onEnter();
        }
    }
    virtual void phaseExit() {
        if (states[phase].onExit) {
            states[phase].onExit();
        }
    }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t * /*sender*/, GUI_event_t event, void *param) override;
};

/*****************************************************************************/
//template definitions

//todo make radio button events behave like normal button
template <class T>
void DialogStateful<T>::windowEvent(EventLock /*has private ctor*/, window_t * /*sender*/, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK: {
        Response response = radio.Click();
        marlin_FSM_response(GetEnumFromPhaseIndex<T>(phase), response);
        break;
    }
    case GUI_event_t::ENC_UP:
        ++radio;
        gui_invalidate();
        break;
    case GUI_event_t::ENC_DN:
        --radio;
        gui_invalidate();
        break;
    default:
        break;
    }
}
