/*
 * screen_mesh_bed_lv.cpp
 *
 *  Created on: 2019-09-26
 *      Author: Radek Vana
 */

#include "screen_mesh_bed_lv.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "math.h"
#include "marlin_client.hpp"

static const char *btnMeshStrings[] = { "Run mesh", "Mesh in progress" };
#define btnMeshStrings_sz (sizeof(btnMeshStrings) / sizeof(const char *))

static const char *meshStrings[] = { "Mesh not in failed state", "Mesh in failed state" };
#define meshStrings_sz (sizeof(meshStrings) / sizeof(const char *))
//-----------------------------------------------------------------------------
// methods

static const constexpr color_t MESH_DEFAULT_CL = COLOR_WHITE;
static const constexpr color_t MESH_ACTIVE_CL = COLOR_RED;

// mesh callbacks
void screen_mesh_bed_lv_data_t::gui_state_mesh_off() {
    btMesh.SetTextColor(MESH_DEFAULT_CL);
    btMesh.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)btnMeshStrings[0]));
    textExit.SetTextColor(MESH_DEFAULT_CL);
    textExit.Enable();
    text_mesh_state.Enable();
}

void screen_mesh_bed_lv_data_t::gui_state_mesh_on() {
    textExit.Disable();
    textExit.SetTextColor(MESH_ACTIVE_CL);
    text_mesh_state.Disable();
    btMesh.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)btnMeshStrings[1]));
    btMesh.SetTextColor(MESH_ACTIVE_CL);
}

static constexpr uint16_t row_h = 25;
mesh_state_t screen_mesh_bed_lv_data_t::mesh_state = mesh_state_t::idle;

screen_mesh_bed_lv_data_t::screen_mesh_bed_lv_data_t()
    : AddSuperWindow<screen_t>()
    , footer(this)
    , textMenuName(this, Rect16(0, 0, display::GetW(), row_h), is_multiline::no)
    , btMesh(this, Rect16(2, 50, 200, row_h), []() { if (mesh_state == mesh_state_t::idle){ mesh_state = mesh_state_t::start;
} })
    , text_mesh_state(this, Rect16(2, 75, 200, row_h), is_multiline::no)
    , term(this, { 10, 28 }, &term_buff)
    , textExit(this, Rect16(2, 245, 60, 22), []() {if (mesh_state != mesh_state_t::idle){ return;
} Screens::Access()->Close(); }) {

    textMenuName.set_font(resource_font(IDR_FNT_BIG));
    textMenuName.SetText(_("MESH BED LEVELING"));

    btMesh.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)btnMeshStrings[0]));

    // exit and footer
    textExit.set_font(resource_font(IDR_FNT_BIG));
    textExit.SetText(_("EXIT"));
}

void screen_mesh_bed_lv_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        if (marlin_client::error(MARLIN_ERR_ProbingFailed)) {
            text_mesh_state.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)meshStrings[1]));
        } else {
            text_mesh_state.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)meshStrings[0]));
        }
        switch (mesh_state) {
        case mesh_state_t::idle:
            // do nothing
            break;
        case mesh_state_t::start:
            gui_state_mesh_on();
            mesh_state = mesh_state_t::home;
            break;
        case mesh_state_t::home:
            marlin_client::error_clr(MARLIN_ERR_ProbingFailed);
            marlin_client::event_clr(marlin_server::Event::CommandBegin);
            marlin_client::event_clr(marlin_server::Event::CommandEnd);
            marlin_client::gcode_printf("G28");
            while (!marlin_client::event_clr(marlin_server::Event::CommandBegin)) {
                marlin_client::loop();
            }
            mesh_state = mesh_state_t::homeing;
            break;
        case mesh_state_t::homeing:
            if (marlin_client::event_clr(marlin_server::Event::CommandEnd)) {
                mesh_state = mesh_state_t::homed;
            }
            break;
        case mesh_state_t::homed:
            mesh_state = mesh_state_t::mesh;
            [[fallthrough]];
        case mesh_state_t::mesh:
            marlin_client::event_clr(marlin_server::Event::CommandBegin);
            marlin_client::event_clr(marlin_server::Event::CommandEnd);
            marlin_client::gcode_printf("G29");
            while (!marlin_client::event_clr(marlin_server::Event::CommandBegin)) {
                marlin_client::loop();
            }
            mesh_state = mesh_state_t::meshing;
            break;
        case mesh_state_t::meshing:
            if (marlin_client::event_clr(marlin_server::Event::CommandEnd)) {
                mesh_state = mesh_state_t::meshed;
            }
            break;
        case mesh_state_t::meshed:
            gui_state_mesh_off();
            mesh_state = mesh_state_t::idle;
            break;
        }
    }
    SuperWindowEvent(sender, event, param);
}
