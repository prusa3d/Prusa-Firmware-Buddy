/*
 * screen_mesh_bed_lv.cpp
 *
 *  Created on: 2019-09-26
 *      Author: Radek Vana
 */

#include "screen_mesh_bed_lv.hpp"
#include "config.h"

#include "math.h"
#include "marlin_client.h"
#include "../../lang/i18n.h"

static const char *btnMeshStrings[] = { "Run mesh", "Mesh in progress" };
#define btnMeshStrings_sz (sizeof(btnMeshStrings) / sizeof(const char *))

static const char *meshStrings[] = { "Mesh not in failed state", "Mesh in failed state" };
#define meshStrings_sz (sizeof(meshStrings) / sizeof(const char *))
//-----------------------------------------------------------------------------
//methods

#define MESH_DEFAULT_CL COLOR_WHITE
#define MESH_ACTIVE_CL  COLOR_RED

//mesh callbacks
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

enum {
    TAG_QUIT = 10,
    TAG_MESH

};

static constexpr uint16_t row_h = 25;

screen_mesh_bed_lv_data_t::screen_mesh_bed_lv_data_t()
    : window_frame_t(&footer)
    , footer(this)
    , textMenuName(this, rect_ui16(0, 0, display::GetW(), row_h))
    , btMesh(this, rect_ui16(2, 50, 200, row_h))
    , text_mesh_state(this, rect_ui16(2, 75, 200, row_h))
    , term(this, rect_ui16(10, 28, 11 * 20, 18 * 16))
    //, terminal(this, )
    , textExit(this, rect_ui16(2, 245, 60, 22))
    , mesh_state(mesh_state_t::idle) {

    textMenuName.font = resource_font(IDR_FNT_BIG);
    textMenuName.SetText(_("MESH BED L."));

    btMesh.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)btnMeshStrings[0]));
    btMesh.Enable();
    btMesh.SetTag(TAG_MESH);

    //terminal
    term_init(&(terminal), 20, 16, term_buff);
    term.term = &(terminal);

    //exit and footer
    textExit.font = resource_font(IDR_FNT_BIG);
    textExit.SetText(_("EXIT"));
    textExit.Enable();
    textExit.SetTag(TAG_QUIT);
}

int screen_mesh_bed_lv_data_t::event(window_t *sender, uint8_t event, void *param) {
    /* if (status_footer_event(&(footer), window, event, param)) {
        return 1;
    }*/

    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case TAG_QUIT:
            if (mesh_state != mesh_state_t::idle)
                return 0; //button should not be accessible
            screen_close();
            return 1;

        case TAG_MESH:
            if (mesh_state == mesh_state_t::idle) {
                gui_state_mesh_on();
                mesh_state = mesh_state_t::home;
            }
            break;
        }
    if (event == WINDOW_EVENT_CHANGE) {
        switch ((int)param) {
        case TAG_MESH:
            break;
        }
    }
    if (event == WINDOW_EVENT_LOOP) {
        if (marlin_error(MARLIN_ERR_ProbingFailed)) {
            text_mesh_state.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)meshStrings[1]));
        } else {
            text_mesh_state.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)meshStrings[0]));
        }
        switch (mesh_state) {
        case mesh_state_t::idle:
            //do nothing
            break;
        case mesh_state_t::home:
            marlin_error_clr(MARLIN_ERR_ProbingFailed);
            marlin_event_clr(MARLIN_EVT_CommandBegin);
            marlin_event_clr(MARLIN_EVT_CommandEnd);
            marlin_gcode_printf("G28");
            while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
                marlin_client_loop();
            mesh_state = mesh_state_t::homeing;
            break;
        case mesh_state_t::homeing:
            if (marlin_event_clr(MARLIN_EVT_CommandEnd)) {
                mesh_state = mesh_state_t::homed;
            }
            break;
        case mesh_state_t::homed:
            mesh_state = mesh_state_t::mesh;
            //there is no break;
        case mesh_state_t::mesh:
            marlin_event_clr(MARLIN_EVT_CommandBegin);
            marlin_event_clr(MARLIN_EVT_CommandEnd);
            marlin_gcode_printf("G29");
            while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
                marlin_client_loop();
            mesh_state = mesh_state_t::meshing;
            break;
        case mesh_state_t::meshing:
            if (marlin_event_clr(MARLIN_EVT_CommandEnd)) {
                mesh_state = mesh_state_t::meshed;
            }
            break;
        case mesh_state_t::meshed:
            gui_state_mesh_off();
            mesh_state = mesh_state_t::idle;
            break;
        }
    }

    return 0;
}
