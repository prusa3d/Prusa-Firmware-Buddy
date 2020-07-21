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

enum class mesh_state_t : uint8_t {
    idle,
    home,
    homeing,
    homed,
    mesh,
    meshing,
    meshed
};

struct screen_mesh_bed_lv_data_t : public window_frame_t {
    status_footer_t footer;
    window_text_t textMenuName;
    window_text_t btMesh;
    window_text_t text_mesh_state;
    window_term_t term;
    term_t terminal;
    uint8_t term_buff[TERM_BUFF_SIZE(20, 16)]; //chars and attrs (640 bytes) + change bitmask (40 bytes)
    window_text_t textExit;
    mesh_state_t mesh_state;

public:
    screen_mesh_bed_lv_data_t();

private:
    virtual int windowEvent(window_t *sender, uint8_t event, void *param) override;

    void gui_state_mesh_off();
    void gui_state_mesh_on();
};
