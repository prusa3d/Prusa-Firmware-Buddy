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

struct screen_test_term_data_t : public AddSuperWindow<window_frame_t> {
    window_text_t text;
    window_term_t term;
    term_buff_t<20, 16> term_buff;

public:
    screen_test_term_data_t();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
