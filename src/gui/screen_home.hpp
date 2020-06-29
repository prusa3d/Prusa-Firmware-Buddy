//screen_home.hpp
#include "window_header.hpp"
#include "status_footer.h"
#include "gui.hpp"

int screen_home_event(screen_t *screen, window_t *window, uint8_t event, void *param);
void screen_home_done(screen_t *screen);
void screen_home_draw(screen_t *screen);
void screen_home_init(screen_t *screen);

struct screen_home_data_t : public window_frame_t {
    window_header_t header;
    window_icon_t logo;

    window_icon_t w_buttons[6];
    window_text_t w_labels[6];

    status_footer_t footer;

    uint8_t is_starting;
    uint32_t time;
    uint8_t logo_invalid;

    screen_home_data_t() {};
};
