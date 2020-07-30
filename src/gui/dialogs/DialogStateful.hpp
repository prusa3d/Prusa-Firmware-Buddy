#pragma once

#include "IDialog.hpp"
#include <array>
#include "DialogRadioButton.hpp"
#include "marlin_client.hpp"
#include "client_response.hpp"
#include "../lang/i18n.h"
#include "window_text.hpp"

//#define DLG_FRAME_ENA 1
#define DLG_FRAME_ENA 0

// function pointer for onEnter & onExit callbacks
using change_state_cb_t = void (*)();

//abstract parent containing general code for any number of phases
class IDialogStateful : public IDialog {
protected:
    //dialog flags bitshift
    enum {
        DLG_SHI_MOD = 4, // mode shift
        DLG_SHI_CHG = 14 // change flag shift
    };
    enum : uint32_t {
#if DLG_FRAME_ENA == 1
        DLG_DRA_FR = 0x0800, // draw frame
#else
        DLG_DRA_FR = 0x0000, // draw frame
#endif                                          //DLG_FRAME_ENA == 1
        DLG_TXT_CH = 0x2000,                    // text changed
        DLG_PRO_CH = 0x4000,                    // progress changed
        DLG_PPR_CH = 0x8000,                    // part progress changed
        DLG_PRX_CH = (DLG_PRO_CH | DLG_PPR_CH), // some progress changed
        DLG_PHA_CH = (DLG_PRX_CH | DLG_TXT_CH), // phase changed
        //dialog flags bitmasks
        DLG_MSK_MOD = 0x0003,    // mode mask
        DLG_MSK_CHG = DLG_PHA_CH // change flag mask
    };

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
    //window_text_t label;
    RadioButton radio;
    color_t color_text;
    font_t *font;
    font_t *font_title;
    padding_ui8_t padding;
    uint16_t flags;
    uint8_t last_text_h; //hack todo remove me
    uint8_t phase;
    uint8_t progress;

    virtual bool can_change(uint8_t phase) = 0;
    // must be virtual because of `states` list is in template protected
    virtual void phaseEnter() = 0;
    virtual void phaseExit() = 0;

public:
    IDialogStateful(string_view_utf8 name);
    bool Change(uint8_t phs, uint8_t progress_tot, uint8_t progress); // = 0; todo should be pure virtual

protected:
    void draw_phase_text(string_view_utf8 text);
    void draw_frame();
    void draw_progress();
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
        if (states[phase].onEnter) {
            states[phase].onEnter();
        }
    }
    virtual void phaseExit() {
        if (states[phase].onExit) {
            states[phase].onExit();
        }
    }

public:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(window_t * /*sender*/, uint8_t event, void *param) override;
};

/*****************************************************************************/
//template definitions

template <class T>
void DialogStateful<T>::unconditionalDraw() {

    const char *text = states[phase].label;
    if (IsInvalid()) {
        Validate();
        flags |= DLG_DRA_FR | DLG_PHA_CH | DLG_PPR_CH;
    }

    //button knows when it needs to be repainted except when phase changes
    //todo check if button repaints
    /*if (flags & DLG_PHA_CH) {
        //do not clear DLG_PHA_CH
        radio.unconditionalDraw();
    } else
        radio.Draw();*/

    if (flags & DLG_TXT_CH) //text changed
    {
        draw_phase_text(_(text));
        flags &= ~DLG_TXT_CH;
    }

    if (flags & DLG_PRX_CH) //any progress changed
    {
        draw_progress();
        flags &= ~DLG_PRX_CH;
    }
    if (flags & DLG_DRA_FR) { //draw frame
        draw_frame();
        flags &= ~DLG_DRA_FR;
    }
}

//todo make radi button events behave like normal button
template <class T>
void DialogStateful<T>::windowEvent(window_t * /*sender*/, uint8_t event, void *param) {
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
    case WINDOW_EVENT_CLICK: {
        Response response = radio.Click();
        marlin_FSM_response(GetEnumFromPhaseIndex<T>(phase), response);
        break;
    }
    case WINDOW_EVENT_ENC_UP:
        ++radio;
        gui_invalidate();
        break;
    case WINDOW_EVENT_ENC_DN:
        --radio;
        gui_invalidate();
        break;
    }
}
