//screen_printing_serial.hpp
#pragma once
#include "IScreenPrinting.hpp"
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.h"

enum class buttons_t {
    TUNE = 0,
    PAUSE,
    DISCONNECT,
    count
};

class screen_printing_serial_data_t : public IScreenPrinting {
    static constexpr const char caption[] = "SERIAL PRT.";
    static constexpr btn_resource res_disconnect = { IDR_PNG_menu_icon_disconnect, N_("Disconnect") };

    window_icon_t octo_icon;

    int last_tick;
    enum class connection_state_t { connected,
        disconnect,
        disconnecting,
        disconnected };
    static connection_state_t connection;

public:
    screen_printing_serial_data_t();

private:
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
    virtual void unconditionalDraw() override;
    void DisableButton(btn &b);
};
