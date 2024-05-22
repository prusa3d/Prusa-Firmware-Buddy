// screen_printing_serial.hpp
#pragma once
#include "ScreenPrintingModel.hpp"
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "window_text.hpp"
#include <array>

class screen_printing_serial_data_t : public AddSuperWindow<ScreenPrintingModel> {
    static constexpr const char *caption = N_("SERIAL PRINTING");

    window_icon_t octo_icon;

    int last_tick;
    enum class connection_state_t { connected,
        disconnect,
        disconnect_ask,
        disconnecting,
        disconnected };
    connection_state_t connection;

    marlin_server::State last_state;

public:
    screen_printing_serial_data_t();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    virtual void stopAction() override;
    virtual void pauseAction() override;
    virtual void tuneAction() override;
};
