#pragma once
#include "gui.hpp"
#include "screen.hpp"

class screen_splash_data_t : public AddSuperWindow<screen_t> {
#if defined(USE_ST7789)
    img::ResourceSingleFile png_printer;
    img::ResourceSingleFile png_marlin;

    window_icon_t icon_logo_printer;
    window_text_t text_progress;
    window_progress_t progress;
    window_text_t text_version;
    window_icon_t icon_logo_marlin;
#endif // USE_7789

#if defined(USE_ILI9488)
    window_text_t text_version;
    window_numberless_progress_t progress;
    window_text_t text_progress;
#endif // USE_ILI9488

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
