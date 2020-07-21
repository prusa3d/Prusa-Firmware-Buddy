// firstlay.c

#include "firstlay.h"
#include "dbg.h"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "marlin_client.h"
#include "wizard_config.h"
#include "wizard_ui.h"
#include "wizard_types.h"
#include "wizard_progress_bar.h"
#include "guitypes.h" //font_meas_text
#include "menu_vars.h"
#include "filament.h"
#include "../lang/i18n.h"
#include "gcode.cpp"

void initialGcodes() {
    gCode gc;
    gc.M(107)  // fan off
        .G(90) // use absolute coordinates
        .M(83) // extruder relative mode
        .G(21) // set units to millimeters
        .M(83) // use relative distances for extrusion
        .send();
}

void homeAndMBL(const uint16_t nozzle_preheat, const uint16_t nozzle_target, const uint8_t bed) {
    gCode gc;
    //TODO check if Z axis is calibrated, if so, don't call G92
    // .G(92).param('Z', 0)
    // .G1(NAN, NAN, 2, NAN, 1000)
    // clang-format off
    gc  
        .M(104).param('S', nozzle_preheat).param('D', nozzle_target) // nozzle target
        .M(140).param('S', bed)                                      // bed target
        .M(109).param('R', nozzle_preheat)                           // wait for nozzle temp
        .M(190).param('S', bed)                                      // wait for bed temp
        .G(28)                                                       // autohome
        .G(29)                                                       // meshbed leveling
        .send();
    // clang-format on
}

void heatNozzle(const uint16_t nozzle_target) {
    gCode gc;
    // clang-format off
    gc   .M(104).param('S', nozzle_target)  // nozzle target
         .M(109).param('S', nozzle_target) // wait for nozzle temp
         .send();
    // clang-format on
}

void firstLayer01(gCode &gc) {
    gc.G1(NAN, NAN, 4, NAN, 1000)
        .G1(0, -2, 0.2, NAN, 3000)
        .G1(NAN, NAN, NAN, 6, 2000)
        .G1(60, NAN, NAN, 9, 1000)
        .G1(100, NAN, NAN, 12.5, 1000)
        .G1(NAN, NAN, 2, -6, 2100);
}

void snakeInit1() {
    gCode gc;
    firstLayer01(gc);
    gc.send();
}

void firstLayer02(gCode &gc) {
    gc.G1(10, 150, 0.2, NAN, 3000)
        .G1(NAN, NAN, NAN, 6, 2000)
        .G1(NAN, NAN, NAN, NAN, 1000);
}

void snakeInit2() {
    gCode gc;
    firstLayer02(gc);
    gc.send();
}

void snakeEnd() {
    gCode gc;
    gc.G1(NAN, NAN, 2, -6, 2100)
        .G1(178, 0, 10, NAN, 3000)
        .G(4)
        .M(107)
        .M(104)
        .param('S', 0)
        .M(140)
        .param('S', 0)
        .M(84)
        .send();
}

static const float snake[][2] = {
    { 10, 150 }, // initialization point, does not extrude
    { 170, 150 },
    { 170, 130 },
    { 10, 130 },
    { 10, 110 },
    { 170, 110 },
    { 170, 90 },
    { 10, 90 },
    { 10, 70 },
    { 170, 70 },
    { 170, 50 },
    { 10, 50 },

    /// frame
    { 10, 17 },
    { 31, 17 },
    { 31, 30.5 },
    { 10.5, 30.5 },
    { 10.5, 30.0 },

    /// zig-zag
    { 30.5, 30.0 },
    { 30.5, 29.5 },
    { 10.5, 29.5 },
    { 10.5, 29.0 },
    { 30.5, 29.0 },
    { 30.5, 28.5 },
    { 10.5, 28.5 },
    { 10.5, 28.0 },
    { 30.5, 28.0 },
    { 30.5, 27.5 },
    { 10.5, 27.5 },
    { 10.5, 27.0 },
    { 30.5, 27.0 },
    { 30.5, 26.5 },
    { 10.5, 26.5 },
    { 10.5, 26.0 },
    { 30.5, 26.0 },
    { 30.5, 25.5 },
    { 10.5, 25.5 },
    { 10.5, 25.0 },
    { 30.5, 25.0 },
    { 30.5, 24.5 },
    { 10.5, 24.5 },
    { 10.5, 24.0 },
    { 30.5, 24.0 },
    { 30.5, 23.5 },
    { 10.5, 23.5 },
    { 10.5, 23.0 },
    { 30.5, 23.0 },
    { 30.5, 22.5 },
    { 10.5, 22.5 },
    { 10.5, 22.0 },
    { 30.5, 22.0 },
    { 30.5, 21.5 },
    { 10.5, 21.5 },
    { 10.5, 21.0 },
    { 30.5, 21.0 },
    { 30.5, 20.5 },
    { 10.5, 20.5 },
    { 10.5, 20.0 },
    { 30.5, 20.0 },
    { 30.5, 19.5 },
    { 10.5, 19.5 },
    { 10.5, 19.0 },
    { 30.5, 19.0 },
    { 30.5, 18.5 },
    { 10.5, 18.5 },
    { 10.5, 18.0 },
    { 30.5, 18.0 },
    { 30.5, 17.5 },
    { 10.5, 17.5 }
};

const uint16_t snakeLines = sizeof(snake) / sizeof(snake[0]) - 1;
uint16_t snakeLine = 0;

void sendSnakeLine(uint16_t line) {
    if (line < 0 || line >= snakeLines - 2)
        return;

    gCode gc;
    gc.lastExtrusion(snake[line][0], snake[line][1]);
    gc.ex(snake[line + 1][0], snake[line + 1][1]);
    gc.send();
}

int _get_progress() {
    //if ( _is_gcode_end_line() ) return 100;
    //return std::min(99, int(100 * (line_head + 1 + line_body + 1) / gcode_sz));
    return std::min(99, int(100.0f * snakeLine / (float)snakeLines));
}

//void _set_gcode_first_lines();

//returns remaining lines
static const char *_wizard_firstlay_text = N_("Once the printer   \n"
                                              "starts extruding   \n"
                                              "plastic, adjust    \n"
                                              "the nozzle height  \n"
                                              "by turning the knob\n"
                                              "until the filament \n"
                                              "sticks to the print\n"
                                              "sheet.");

void _wizard_firstlay_Z_step(firstlay_screen_t *p_screen) {
    //need last step to ensure correct behavior on limits
    const float _step_last = p_screen->Z_offset;
    p_screen->Z_offset += p_screen->Z_offset_request;
    p_screen->Z_offset = std::min(z_offset_max, std::max(z_offset_min, p_screen->Z_offset));

    marlin_do_babysteps_Z(p_screen->Z_offset - _step_last);

    /// change Z offset (Live adjust Z)
    static const char *pm[2] = { "+++", "---" }; /// plus / minus chars
    if (p_screen->Z_offset_request != 0) {
        p_screen->spin_baby_step.SetValue(p_screen->Z_offset);
        p_screen->text_direction_arrow.SetText(
            string_view_utf8::MakeCPUFLASH((const uint8_t *)pm[signbit(p_screen->Z_offset_request)]));
        p_screen->Z_offset_request = 0;
    }
}

void wizard_init_screen_firstlay(int16_t id_body, firstlay_screen_t *p_screen, firstlay_data_t *p_data) {
    //marlin_vars_t* vars        = marlin_update_vars( MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET) );
    //p_screen->Z_offset         = vars->z_offset;
    p_screen->Z_offset_request = 0;

    window_destroy_children(id_body);
    window_t *pWin = window_ptr(id_body);
    if (pWin != 0) {
        pWin->Show();
        pWin->Invalidate();
    }
    uint16_t y = 40;
    uint16_t x = WIZARD_MARGIN_LEFT;
#if DEBUG_TERM == 0
    point_ui16_t pt;
    string_view_utf8 wft = string_view_utf8::MakeCPUFLASH((const uint8_t *)_wizard_firstlay_text);
    uint16_t numOfUTF8Chars = 0;
    pt = font_meas_text(resource_font(IDR_FNT_NORMAL), &wft, &numOfUTF8Chars);
    pt.x += 5;
    pt.y += 5;
    window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, pt.x, pt.y), &(p_screen->text_state));
    p_screen->text_state.font = resource_font(IDR_FNT_NORMAL);
    wft.rewind();
    p_screen->text_state.SetText(wft);

    y += pt.y + 5;
#else
    window_create_ptr(WINDOW_CLS_TERM, id_body,
        rect_ui16(10, y,
            resource_font(IDR_FNT_SMALL)->w * FIRSTLAY_SCREEN_TERM_X,
            resource_font(IDR_FNT_SMALL)->h * FIRSTLAY_SCREEN_TERM_Y),
        &(p_screen->term));
    p_screen->term.font = resource_font(IDR_FNT_SMALL);
    term_init(&(p_screen->terminal), FIRSTLAY_SCREEN_TERM_X, FIRSTLAY_SCREEN_TERM_Y, p_screen->term_buff);
    p_screen->term.term = &(p_screen->terminal);

    y += 18 * FIRSTLAY_SCREEN_TERM_Y + 3;
#endif
    window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, 110, 22), &(p_screen->text_Z_pos));
    p_screen->text_Z_pos.SetText(_("Z height:"));

    window_create_ptr(WINDOW_CLS_NUMB, id_body, rect_ui16(x + 110, y, 70, 22), &(p_screen->spin_baby_step));
    p_screen->spin_baby_step.SetFormat("%.3f");
    p_screen->spin_baby_step.SetValue(p_screen->Z_offset);
    p_screen->spin_baby_step.color_text = COLOR_GRAY;

    window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x + 110 + 70, y, WIZARD_X_SPACE - x - 110 - 70, 22),
        &(p_screen->text_direction_arrow));
    static const char pm[] = "-|+";
    p_screen->text_direction_arrow.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)pm));

    y += 22 + 10;

    window_create_ptr(WINDOW_CLS_PROGRESS, id_body, rect_ui16(x, y, WIZARD_X_SPACE, 8), &(p_screen->progress));
}

inline float targetTemp() {
    return get_filament_nozzle_temp();
}

inline float preheatTemp() {
    return PREHEAT_TEMP;
}

inline float bedTemp() {
    return get_filament_bed_temp();
}

inline void FLInit(int16_t id_body, firstlay_screen_t *p_screen, firstlay_data_t *p_data, float z_offset) {
    p_screen->Z_offset = z_offset;
    wizard_init_screen_firstlay(id_body, p_screen, p_data);
    initialGcodes();

#if DEBUG_TERM == 1
    term_printf(&p_screen->terminal, "INITIALIZED\n");
    p_screen->term.id.Invalidate();
#endif
    //_set_gcode_first_lines();
    p_screen->state = _FL_GCODE_MBL;
    marlin_error_clr(MARLIN_ERR_ProbingFailed);
#if DEBUG_TERM == 1
    term_printf(&p_screen->terminal, "HEAD\n");
    p_screen->term.id.Invalidate();
#endif
}

inline void FLGcodeHeat(firstlay_screen_t *p_screen) {
    if (marlin_get_gqueue() > 0)
        return;

    heatNozzle(targetTemp());
    p_screen->state = _FL_GCODE_SNAKE_INIT_1;

    p_screen->Z_offset_request = 0; //ignore Z_offset_request variable changes until now
    p_screen->spin_baby_step.color_text = COLOR_ORANGE;
    p_screen->spin_baby_step.Invalidate();
}

int wizard_firstlay_print(int16_t id_body, firstlay_screen_t *p_screen, firstlay_data_t *p_data, float z_offset) {
    if (p_data->state_print == _TEST_START) {
        p_screen->state = _FL_INIT;
        p_data->state_print = _TEST_RUN;
    }

    switch (p_screen->state) {
    case _FL_INIT:
        FLInit(id_body, p_screen, p_data, z_offset);
        break;

    case _FL_GCODE_MBL:
        if (marlin_get_gqueue() > 0)
            break;

        homeAndMBL(preheatTemp(), targetTemp(), bedTemp());
        p_screen->state = _FL_GCODE_HEAT;
        break;

    case _FL_GCODE_HEAT:
        /// check MBL final status
        if (marlin_error(MARLIN_ERR_ProbingFailed)) {
            marlin_error_clr(MARLIN_ERR_ProbingFailed);
            marlin_gcode("G0 Z30"); //Z 30mm
            marlin_gcode("M84");    //Disable steppers
            if (wizard_msgbox(_("Mesh bed leveling failed?"), MSGBOX_BTN_RETRYCANCEL, 0) == MSGBOX_RES_RETRY) {
                p_screen->state = _FL_GCODE_MBL; // retry
            } else {
                p_data->state_print = _TEST_FAILED; /// abort
                return 100;
            }
        }
        FLGcodeHeat(p_screen);
        break;

    case _FL_GCODE_SNAKE_INIT_1:
        if (marlin_get_gqueue() > 0)
            break;

        snakeInit1();
        p_screen->state = _FL_GCODE_SNAKE_INIT_2;
        break;

    case _FL_GCODE_SNAKE_INIT_2:
        if (marlin_get_gqueue() > 0)
            break;

        snakeInit2();
        p_screen->state = _FL_GCODE_SNAKE_INIT_END;
        break;

    case _FL_GCODE_SNAKE_INIT_END:
        if (marlin_get_gqueue() > 0)
            break;

        p_screen->state = _FL_GCODE_SNAKE_BODY;
        break;

    case _FL_GCODE_SNAKE_BODY:
        _wizard_firstlay_Z_step(p_screen);
        sendSnakeLine(snakeLine++);
        if (snakeLine >= snakeLines)
            p_screen->state = _FL_GCODE_SNAKE_FINALIZE;
        break;

    case _FL_GCODE_SNAKE_FINALIZE:
        if (marlin_get_gqueue() > 0)
            break;

        snakeEnd();
        p_screen->state = _FL_GCODE_SNAKE_END;
        break;

    case _FL_GCODE_SNAKE_END:
        if (marlin_get_gqueue() > 0)
            break;
        p_screen->state = _FL_GCODE_DONE;
        break;

    case _FL_GCODE_DONE:
#if DEBUG_TERM == 1
        term_printf(&p_screen->terminal, "PASSED\n");
        p_screen->term.Invalidate();
#endif
        p_data->state_print = _TEST_PASSED;
        p_screen->Z_offset_request = 0;
        return 100;
    }

    int progress = _get_progress(); //max 99

    p_screen->progress.SetValue(progress);
    return progress;
}

void wizard_firstlay_event_dn(firstlay_screen_t *p_screen) {
#if DEBUG_TERM == 1
    //todo term is bugged spinner can make it not showing
    p_screen->term.Invalidate();
#endif
    p_screen->Z_offset_request -= z_offset_step;
}

void wizard_firstlay_event_up(firstlay_screen_t *p_screen) {
#if DEBUG_TERM == 1
    //todo term is bugged spinner can make it not showing
    p_screen->term.Invalidate();
#endif
    p_screen->Z_offset_request += z_offset_step;
}
