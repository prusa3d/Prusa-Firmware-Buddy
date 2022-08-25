//screen_home.hpp
#pragma once
#include "window_header.hpp"
#include "status_footer.hpp"
#include "gui.hpp"
#include "screen.hpp"

class screen_home_data_t : public AddSuperWindow<screen_t> {
public:
    static constexpr size_t button_count = 6;

private:
    static bool usbWasAlreadyInserted; // usb inserted at least once
    static uint32_t lastUploadCount;
    static bool ever_been_openned; //set by ctor
    static bool try_esp_flash;     // we try this maximum once

    bool usbInserted;
    bool esp_flash_being_openned;

    window_header_t header;
    StatusFooter footer;

    window_icon_t logo;
    window_icon_button_t w_buttons[button_count];
    window_text_t w_labels[button_count];

    GCodeInfo &gcode;

public:
    static bool EverBeenOpenned() { return ever_been_openned; }
    static bool MoreGcodesUploaded();
    screen_home_data_t();
    virtual ~screen_home_data_t() override;

    virtual void InitState(screen_init_variant var) override;
    virtual screen_init_variant GetCurrentState() const override;

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    virtual void draw() override;

    void printBtnEna();
    void printBtnDis();
};
