// screen_mesh_bed_lv.hpp
#pragma once
#include "gui.hpp"
#include "status_footer.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "window_term.hpp"
#include "window_progress.hpp"
#include "screen.hpp"

enum class mesh_state_t : uint8_t {
    idle,
    start,
    home,
    homeing,
    homed,
    mesh,
    meshing,
    meshed
};

struct screen_mesh_bed_lv_data_t : public AddSuperWindow<screen_t> {
    StatusFooter footer;
    window_text_t textMenuName;
    window_text_button_t btMesh;
    window_text_t text_mesh_state;
    window_term_t term;
    term_buff_t<20, 16> term_buff;
    window_text_button_t textExit;
    static mesh_state_t mesh_state;

public:
    screen_mesh_bed_lv_data_t();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    void gui_state_mesh_off();
    void gui_state_mesh_on();
};
