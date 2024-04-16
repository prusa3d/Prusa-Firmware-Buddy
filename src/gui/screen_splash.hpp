#pragma once
#include "gui.hpp"
#include "screen.hpp"
#include <guiconfig/guiconfig.h>

class screen_splash_data_t : public AddSuperWindow<screen_t> {
#if defined(USE_ST7789)
    img::ResourceSingleFile img_printer;
    img::ResourceSingleFile img_marlin;

    window_icon_t icon_logo_printer;
    window_icon_t icon_logo_marlin;
#endif // USE_7789

    window_text_t text_progress;
    window_numberless_progress_t progress;

    bool version_displayed;
    char text_progress_buffer[32];

public:
    screen_splash_data_t();
    virtual ~screen_splash_data_t();

    static void bootstrap_cb(unsigned percent, std::optional<const char *> str);

private:
    virtual void draw() override;

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
