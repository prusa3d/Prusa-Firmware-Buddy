//screen_home.hpp
#pragma once
#include "window_header.hpp"
#include "status_footer.h"
#include "gui.hpp"
#include "screen.hpp"

struct screen_home_data_t : public AddSuperWindow<screen_t> {
    static bool usbWasAlreadyInserted; // usb inserted at least once
    bool usbInserted;

    window_header_t header;
    status_footer_t footer;

    window_icon_t logo;
    window_icon_button_t w_buttons[6];
    window_text_t w_labels[6];

    screen_home_data_t();
    virtual ~screen_home_data_t() override;

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    virtual void draw() override;

    void printBtnEna();
    void printBtnDis();
};
