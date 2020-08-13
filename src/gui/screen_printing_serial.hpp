//screen_printing_serial.hpp
#pragma once
#include "IScreenPrinting.hpp"
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.h"
#include "window_text.hpp"
#include <array>

static constexpr btn_resource res_disconnect = { IDR_PNG_menu_icon_disconnect, N_("Disconnect") };

class screen_printing_serial_data_t : public IScreenPrinting {
    static constexpr const char *caption = "SERIAL PRT.";
    // static constexpr btn_resource res_disconnect = { IDR_PNG_menu_icon_disconnect, N_("Disconnect") };

    window_icon_t octo_icon;

    int last_tick;
    enum class connection_state_t { connected,
        disconnect,
        disconnect_ask,
        disconnecting,
        disconnected };
    connection_state_t connection;

public:
    screen_printing_serial_data_t();

private:
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
    void DisableButton(btn &b);

    virtual void stopAction() override;
    virtual void pauseAction() override;
    virtual void tuneAction() override;
};
