#pragma once

#include <i_window_menu_item.hpp>
#include <marlin_client.hpp>

/// Window menu item that sends an FSM response
class FSMMenuItem : public IWindowMenuItem {

public:
    template <typename Phase>
    FSMMenuItem(Phase phase, Response response, const string_view_utf8 &text, const img::Resource *icon = nullptr)
        : IWindowMenuItem(text, icon)
        , encoded_response(EncodedFSMResponse::encode(phase, FSMResponseVariant::make(response))) //
    {}

protected:
    virtual void click(IWindowMenu &) {
        marlin_client::FSM_encoded_response(encoded_response);
    }

private:
    EncodedFSMResponse encoded_response;
};
