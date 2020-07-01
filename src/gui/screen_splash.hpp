#include "gui.hpp"

int screen_splash_event(screen_t *screen, window_t *window, uint8_t event, void *param);
void screen_splash_done(screen_t *screen);
void screen_splash_draw(screen_t *screen);
void screen_splash_init(screen_t *screen);

struct screen_splash_data_t : public window_frame_t {
    window_icon_t logo_prusa_mini;
    window_text_t text_progress;
    window_progress_t progress;
    window_text_t text_version;
    char text_version_buffer[16];
    window_icon_t icon_logo_buddy;
    window_icon_t icon_logo_marlin;

    window_icon_t icon_debug;

    uint32_t last_timer;
    uint8_t logo_invalid;

    screen_splash_data_t() {};
};
