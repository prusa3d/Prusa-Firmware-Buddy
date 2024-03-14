#pragma once

#include "IDialog.hpp"
#include <array>
#include <optional>
#include "radio_button_fsm.hpp"
#include "marlin_client.hpp"
#include "client_response.hpp"
#include "i18n.h"
#include "window_text.hpp"
#include "window_progress.hpp"
#include "fsm_types.hpp"

// function pointer for onEnter & onExit callbacks
using change_state_cb_t = void (*)();

class IDialogMarlin : public IDialog {
public:
    virtual bool Change([[maybe_unused]] fsm::BaseData data) { return true; }

    IDialogMarlin(Rect16 rc)
        : IDialog(rc) {}
    IDialogMarlin(std::optional<Rect16> rc = std::nullopt);
};

// abstract parent containing general code for any number of phases
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
    window_frame_t progress_frame;

    window_text_t title;
    window_progress_t progress;
    window_text_t label;

    // must be virtual because of `states` list is in template protected
    virtual void phaseEnter() = 0;
    virtual void phaseExit() = 0;
    virtual float deserialize_progress([[maybe_unused]] fsm::PhaseData data) const { return 0.F; }

    static Rect16 get_frame_rect(Rect16 rect, std::optional<has_footer> dialog_has_footer);
    static Rect16 get_title_rect(Rect16 rect);
    static Rect16 get_progress_rect(Rect16 rect);
    static Rect16 get_label_rect(Rect16 rect, std::optional<has_footer> dialog_has_footer);

public:
    IDialogStateful(string_view_utf8 name, std::optional<has_footer> child_has_footer = std::nullopt);
};

/*****************************************************************************/
// parent for stateful dialogs dialog
// use one of enum class from "client_response.hpp" as T
template <class T>
class DialogStateful : public IDialogStateful {
public:
    static constexpr size_t SZ = CountPhases<T>();
    using States = std::array<State, SZ>;

protected:
    States states; // phase text and radio button + onEnter & onExit cb
    std::optional<T> current_phase = std::nullopt;
    RadioButtonFsm<T> radio;

public:
    DialogStateful(string_view_utf8 name, States st, std::optional<has_footer> child_has_footer = std::nullopt)
        : IDialogStateful(name, child_has_footer)
        , states(st)
        , radio(&progress_frame, (child_has_footer == has_footer::yes) ? GuiDefaults::GetButtonRect_AvoidFooter(GetRect()) : GuiDefaults::GetButtonRect(GetRect()), T::_first) {
        progress_frame.CaptureNormalWindow(radio);
        CaptureNormalWindow(progress_frame);
    }

    bool Change(fsm::BaseData data) override final {
        return change(GetEnumFromPhaseIndex<T>(data.GetPhase()), data.GetData());
    }

protected:
    virtual bool can_change(T phase) { return phase < T::_last; }

    virtual bool change(T phase, fsm::PhaseData data) {
        if (!can_change(phase)) {
            return false;
        }
        if ((!current_phase) || (current_phase != phase)) {
            phaseExit();
            current_phase = phase;
            phaseEnter();
        }

        progress.SetValue(deserialize_progress(data));
        return true;
    }

    State &get_current_state() {
        return states[uint8_t(*current_phase)];
    }

    // get arguments callbacks and call them
    virtual void phaseEnter() {
        if (!current_phase) {
            return;
        }

        radio.Change(*current_phase /*, states[phase].btn_resp, &states[phase].btn_labels*/); // TODO alternative button label support
        label.SetText(_(get_current_state().label));
        if (get_current_state().onEnter) {
            get_current_state().onEnter();
        }
    }
    virtual void phaseExit() {
        if (!current_phase) {
            return;
        }

        if (get_current_state().onExit) {
            get_current_state().onExit();
        }
    }
};
