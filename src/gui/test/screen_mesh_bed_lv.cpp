/*
 * screen_mesh_bed_lv.c
 *
 *  Created on: 2019-09-26
 *      Author: Radek Vana
 */

#include "gui.h"
#include "config.h"
#include "status_footer.h"
#include "math.h"
#include "marlin_client.h"

#pragma pack(push)
#pragma pack(1)

typedef enum {
    MS_idle,
    MS_home,
    MS_homeing,
    MS_homed,
    MS_mesh,
    MS_meshing,
    MS_meshed //,
    //MS_done
} mesh_state_t;

typedef struct
{
    window_frame_t frame;
    window_text_t textMenuName;

    window_text_t btMesh;
    window_text_t text_mesh_state;

    window_term_t term;

    int16_t id_term;
    term_t terminal;
    uint8_t term_buff[TERM_BUFF_SIZE(20, 16)]; //chars and attrs (640 bytes) + change bitmask (40 bytes)

    window_text_t textExit;
    status_footer_t footer;

    mesh_state_t mesh_state;
    int exit_bt_id;
    int mesh_bt_id;

} screen_mesh_bed_lv_data_t;

#pragma pack(pop)

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
    //if (pd->mesh_bt_id == -1)return;
    //if (pd->exit_bt_id == -1)return;
    window_set_color_text(pd->mesh_bt_id, MESH_DEFAULT_CL);
    window_set_text(pd->mesh_bt_id, btnMeshStrings[0]);
    window_set_color_text(pd->exit_bt_id, MESH_DEFAULT_CL);
    window_enable(pd->exit_bt_id);
    window_enable(pd->mesh_bt_id);
    //pd->mesh_state = MS_idle;
}

static void gui_state_mesh_on(screen_t *screen) {
    window_disable(pd->exit_bt_id);
    window_set_color_text(pd->exit_bt_id, MESH_ACTIVE_CL);
    window_disable(pd->mesh_bt_id);
    window_set_text(pd->mesh_bt_id, btnMeshStrings[1]);
    window_set_color_text(pd->mesh_bt_id, MESH_ACTIVE_CL);
}

enum {
    TAG_QUIT = 10,
    TAG_MESH

};

void screen_mesh_bed_lv_init(screen_t *screen) {
    pd->mesh_state = MS_idle;
    pd->exit_bt_id = -1;
    pd->mesh_bt_id = -1;

    int16_t id;
    uint16_t row_h = 25;

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME,
        -1, rect_ui16(0, 0, 0, 0), &(pd->frame));

    id = window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(0, 0, display->w, row_h), &(pd->textMenuName));
    pd->textMenuName.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char *)"MESH LEVELING");

    id = window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(2, 50, 200, row_h), &(pd->btMesh));
    window_set_text(id, btnMeshStrings[0]);
    window_enable(id);
    window_set_tag(id, TAG_MESH);

    id = window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(2, 75, 200, row_h),
        &(pd->text_mesh_state));

    //terminal
    id = window_create_ptr(WINDOW_CLS_TERM, id0, rect_ui16(10, 28, 11 * 20, 18 * 16), &(pd->term));
    pd->id_term = id;
    term_init(&(pd->terminal), 20, 16, pd->term_buff);
    pd->term.term = &(pd->terminal);

    //exit and footer

    id = window_create_ptr(WINDOW_CLS_TEXT,
        id0, rect_ui16(2, 245, 60, 22), &(pd->textExit));
    pd->textExit.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char *)"EXIT");
    window_enable(id);
    window_set_tag(id, TAG_QUIT);
    pd->exit_bt_id = id;

    status_footer_init(&(pd->footer), id0);
}

void screen_mesh_bed_lv_done(screen_t *screen) {
    window_destroy(pd->frame.win.id);
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
            if (pd->mesh_state != MS_idle)
                return 0; //button should not be accessible
            screen_close();
            return 1;

        case TAG_MESH:
            if (pd->mesh_state == MS_idle) {
                gui_state_mesh_on(screen);
                pd->mesh_state = MS_home;
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
            window_set_text(pd->text_mesh_state.win.id, meshStrings[1]);
        } else {
            window_set_text(pd->text_mesh_state.win.id, meshStrings[0]);
        }
        switch (pd->mesh_state) {
        case MS_idle:
            //do nothing
            break;
        case MS_home:
            marlin_error_clr(MARLIN_ERR_ProbingFailed);
            marlin_gcode_printf("G28");
            pd->mesh_state = MS_homeing;
            break;
        case MS_homeing:
            if (marlin_event(MARLIN_EVT_Ready)) {
                pd->mesh_state = MS_homed;
                marlin_event_clr(MARLIN_EVT_Ready);
            }
            break;
        case MS_homed:
            pd->mesh_state = MS_mesh;
            //there is no break;
        case MS_mesh:
            marlin_gcode_printf("G29");
            pd->mesh_state = MS_meshing;
            break;
        case MS_meshing:
            if (marlin_event(MARLIN_EVT_Ready)) {
                pd->mesh_state = MS_meshed;
                marlin_event_clr(MARLIN_EVT_Ready);
            }
            break;
        case MS_meshed:
            gui_state_mesh_off(screen);
            pd->mesh_state = MS_idle;
            break;
        }
    }

    return 0;
}

extern "C" {

screen_t screen_mesh_bed_lv = {
    0,
    0,
    screen_mesh_bed_lv_init,
    screen_mesh_bed_lv_done,
    screen_mesh_bed_lv_draw,
    screen_mesh_bed_lv_event,
    sizeof(screen_mesh_bed_lv_data_t), //data_size
    0,                                 //pdata
};

screen_t *const get_scr_mesh_bed_lv() { return &screen_mesh_bed_lv; }
}
