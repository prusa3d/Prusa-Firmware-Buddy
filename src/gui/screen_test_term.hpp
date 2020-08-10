//screen_mesh_bed_lv.hpp
#pragma once
#include "gui.hpp"
#include "status_footer.h"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "window_spin.hpp"
#include "window_list.hpp"
#include "window_term.hpp"
#include "window_progress.hpp"

struct screen_test_term_data_t : public window_frame_t {
    window_text_t text;
    window_term_t term;
    term_t terminal;
    uint8_t term_buff[TERM_BUFF_SIZE(20, 16)]; //chars and attrs (640 bytes) + change bitmask (40 bytes)
public:
    screen_test_term_data_t();

private:
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
};
