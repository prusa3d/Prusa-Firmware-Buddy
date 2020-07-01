/*
 * screen_mesh_bed_lv.c
 *
 *  Created on: 2019-09-26
 *      Author: Radek Vana
 */

#include "gui.hpp"
#include "config.h"
#include "status_footer.h"
#include "math.h"
#include "marlin_client.h"

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
    window_text_t textMenuName;

    window_text_t btMesh;
    window_text_t text_mesh_state;

    window_term_t term;
    term_t terminal;
    uint8_t term_buff[TERM_BUFF_SIZE(20, 16)]; //chars and attrs (640 bytes) + change bitmask (40 bytes)

    window_text_t textExit;
    status_footer_t footer;

    mesh_state_t mesh_state;
    int16_t id_term;
};

#define pd ((screen_mesh_bed_lv_data_t *)screen->pdata)

const char *btnMeshStrings[] = { "Run mesh", "Mesh in progress" };
#define btnMeshStrings_sz (sizeof(btnMeshStrings) / sizeof(const char *))

const char *meshStrings[] = { "Mesh not in failed state", "Mesh in failed state" };
#define meshStrings_sz (sizeof(meshStrings) / sizeof(const char *))
//-----------------------------------------------------------------------------
//methods

#define MESH_DEFAULT_CL COLOR_WHITE
#define MESH_ACTIVE_CL  COLOR_RED

//mesh callbacks
static void gui_state_mesh_off(screen_t *screen) {
    pd->btMesh.SetTextColor(MESH_DEFAULT_CL);
    pd->btMesh.SetText(btnMeshStrings[0]);
    pd->textExit.SetTextColor(MESH_DEFAULT_CL);
    pd->textExit.Enable();
    pd->text_mesh_state.Enable();
}

static void gui_state_mesh_on(screen_t *screen) {
    pd->textExit.Disable();
    pd->textExit.SetTextColor(MESH_ACTIVE_CL);
    pd->text_mesh_state.Disable();
    pd->btMesh.SetText(btnMeshStrings[1]);
    pd->btMesh.SetTextColor(MESH_ACTIVE_CL);
}

enum {
    TAG_QUIT = 10,
    TAG_MESH

};

void screen_mesh_bed_lv_init(screen_t *screen) {
    pd->mesh_state = mesh_state_t::idle;
    uint16_t row_h = 25;

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME,
        -1, rect_ui16(0, 0, 0, 0), pd);

    window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(0, 0, display::GetW(), row_h), &(pd->textMenuName));
    pd->textMenuName.font = resource_font(IDR_FNT_BIG);
    pd->textMenuName.SetText((const char *)"MESH LEVELING");

    window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(2, 50, 200, row_h), &(pd->btMesh));
    pd->btMesh.SetText(btnMeshStrings[0]);
    pd->btMesh.Enable();
    pd->btMesh.SetTag(TAG_MESH);

    window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(2, 75, 200, row_h),
        &(pd->text_mesh_state));

    //terminal
    pd->id_term = window_create_ptr(WINDOW_CLS_TERM, id0, rect_ui16(10, 28, 11 * 20, 18 * 16), &(pd->term));
    term_init(&(pd->terminal), 20, 16, pd->term_buff);
    pd->term.term = &(pd->terminal);

    //exit and footer

    window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(2, 245, 60, 22), &(pd->textExit));
    pd->textExit.font = resource_font(IDR_FNT_BIG);
    pd->textExit.SetText((const char *)"EXIT");
    pd->textExit.Enable();
    pd->textExit.SetTag(TAG_QUIT);

    status_footer_init(&(pd->footer), id0);
}

void screen_mesh_bed_lv_done(screen_t *screen) {
    window_destroy(pd->id);
}

void screen_mesh_bed_lv_draw(screen_t *screen) {
}

int screen_mesh_bed_lv_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (status_footer_event(&(pd->footer), window, event, param)) {
        return 1;
    }

    status_footer_event(&(pd->footer), window, event, param);
    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case TAG_QUIT:
            if (pd->mesh_state != mesh_state_t::idle)
                return 0; //button should not be accessible
            screen_close();
            return 1;

        case TAG_MESH:
            if (pd->mesh_state == mesh_state_t::idle) {
                gui_state_mesh_on(screen);
                pd->mesh_state = mesh_state_t::home;
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
            pd->text_mesh_state.SetText(meshStrings[1]);
        } else {
            pd->text_mesh_state.SetText(meshStrings[0]);
        }
        switch (pd->mesh_state) {
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
            pd->mesh_state = mesh_state_t::homeing;
            break;
        case mesh_state_t::homeing:
            if (marlin_event_clr(MARLIN_EVT_CommandEnd)) {
                pd->mesh_state = mesh_state_t::homed;
            }
            break;
        case mesh_state_t::homed:
            pd->mesh_state = mesh_state_t::mesh;
            //there is no break;
        case mesh_state_t::mesh:
            marlin_event_clr(MARLIN_EVT_CommandBegin);
            marlin_event_clr(MARLIN_EVT_CommandEnd);
            marlin_gcode_printf("G29");
            while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
                marlin_client_loop();
            pd->mesh_state = mesh_state_t::meshing;
            break;
        case mesh_state_t::meshing:
            if (marlin_event_clr(MARLIN_EVT_CommandEnd)) {
                pd->mesh_state = mesh_state_t::meshed;
            }
            break;
        case mesh_state_t::meshed:
            gui_state_mesh_off(screen);
            pd->mesh_state = mesh_state_t::idle;
            break;
        }
    }

    return 0;
}

screen_t screen_mesh_bed_lv = {
    0,
    0,
    screen_mesh_bed_lv_init,
    screen_mesh_bed_lv_done,
    screen_mesh_bed_lv_draw,
    screen_mesh_bed_lv_event,
    sizeof(screen_mesh_bed_lv_data_t), //data_size
    nullptr,                           //pdata
};

screen_t *const get_scr_mesh_bed_lv() { return &screen_mesh_bed_lv; }
