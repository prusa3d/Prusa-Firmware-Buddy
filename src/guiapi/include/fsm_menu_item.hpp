#pragma once

#include <i_window_menu_item.hpp>
#include <marlin_client.hpp>

/// Window menu item that sends an FSM response
class FSMMenuItem : public IWindowMenuItem {

public:
    template <typename Phase>
    FSMMenuItem(Phase phase, Response response, const string_view_utf8 &text, const img::Resource *icon = nullptr)
        : FSMMenuItem(phase, FSMResponseVariant::make(response), text, icon) {}

    template <typename Phase>
    FSMMenuItem(Phase phase, FSMResponseVariant response, const string_view_utf8 &text, const img::Resource *icon = nullptr)
        : IWindowMenuItem(text, icon)
        , encoded_response(EncodedFSMResponse { .response = response, .fsm_and_phase = phase }) //
    {}

protected:
    virtual void click(IWindowMenu &) {
        marlin_client::FSM_encoded_response(encoded_response);
    }

private:
    EncodedFSMResponse encoded_response;
};
