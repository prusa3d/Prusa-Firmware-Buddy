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
    // clang-format off
    gc  .G(92).param('Z', 0)
        .G1(NAN, NAN, 2, NAN, 1000)
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

// void firstLayer03(gCode &gc) {
//     // //E = extrusion_length * layer_height * extrusion_width / (PI * pow(1.75, 2) / 4)
//     gc.lastExtrusion(10, 150)
//         .ex(170, 150)
//         // "G1 X170 Y150 E5.322", //160 * 0.2 * 0.4 / (pi * 1.75 ^ 2 / 4) = 5.322
//         .ex(170, 130)
//         // "G1 X170 Y130 E0.665", //20 * 0.2 * 0.4 / (pi * 1.75 ^ 2 / 4) = 0.665
//         .ex(10, 130)
//         // "G1 X10  Y130 E5.322",
//         .ex(10, 110)
//         // "G1 X10  Y110 E0.665",
//         .ex(170, 110)
//         // "G1 X170 Y110 E5.322",
//         .ex(170, 90);
//     // "G1 X170 Y90  E0.665",
// }

// void firstLayer04(gCode &gc) {
//     gc.lastExtrusion(170, 90)
//         .ex(10, 90)
//         // "G1 X10  Y90  E5.322",
//         .ex(10, 70)
//         // "G1 X10  Y70  E0.665",
//         .ex(170, 70)
//         // "G1 X170 Y70  E5.322",
//         .ex(170, 50)
//         // "G1 X170 Y50  E0.665",
//         .ex(10, 50);
//     // "G1 X10  Y50  E5.322",
// }

// void firstLayer05(gCode &gc) {
//     // //frame around
//     gc.lastExtrusion(10, 50)
//         .ex(10, 17)
//         // "G1 X10    Y17    E1.371975",  //33 * 0.041575 = 1.371975
//         .ex(31, 17)
//         // "G1 X31    Y17    E1.288825",  //31 * 0.041575 = 1.288825
//         .ex(31, 30.5f)
//         // "G1 X31    Y30.5  E0.5612625", //13.5 * 0.041575 = 0.5612625
//         .ex(10.5f, 30.5f)
//         // "G1 X10.5  Y30.5  E0.832",     //20 * 0.2 * 0.5 / (pi * 1.75 ^ 2 / 4) = 0.832
//         .ex(10.5f, 30);
//     // "G1 X10.5  Y30.0  E0.0208",    //0.5 * 0.2 * 0.5 / (pi * 1.75 ^ 2 / 4) = 0.0208
// }

// void firstLayer06(gCode &gc) {
//     gc.G1(NAN, NAN, NAN, NAN, 1000)
//         // "G1 F1000",
//         .lastExtrusion(10.5f, 30)
//         .ex(30.5f, 30)
//         // "G1 X30.5  Y30.0  E0.832",
//         .ex(30.5f, 29.5f)
//         // "G1 X30.5  Y29.5  E0.0208",
//         .ex(10.5f, 29.5f)
//         // "G1 X10.5  Y29.5  E0.832",
//         .ex(10.5f, 29)
//         // "G1 X10.5  Y29.0  E0.0208",
//         .ex(30.5f, 29);
//     // "G1 X30.5  Y29.0  E0.832",
// }

// void firstLayer07(gCode &gc) {
//     gc.lastExtrusion(30.5f, 29)
//         .ex(30.5f, 28.5f)
//         // "G1 X30.5  Y28.5  E0.0208",
//         .ex(10.5, 28.5f)
//         // "G1 X10.5  Y28.5  E0.832",
//         .ex(10.5f, 28)
//         // "G1 X10.5  Y28.0  E0.0208",
//         .ex(30.5f, 28)
//         // "G1 X30.5  Y28.0  E0.832",
//         .ex(30.5f, 27.5f)
//         // "G1 X30.5  Y27.5  E0.0208",
//         .ex(10.5f, 27.5f);
//     // "G1 X10.5  Y27.5  E0.832",
// }

// void firstLayer08(gCode &gc) {
//     gc.lastExtrusion(10.5f, 27.5f)
//         .ex(10.5f, 27)
//         // "G1 X10.5  Y27.0  E0.0208",
//         .ex(30.5f, 27)
//         // "G1 X30.5  Y27.0  E0.832",
//         .ex(30.5f, 26.5f)
//         // "G1 X30.5  Y26.5  E0.0208",
//         .ex(10.5f, 26.5f)
//         // "G1 X10.5  Y26.5  E0.832",
//         .ex(10.5f, 26)
//         // "G1 X10.5  Y26.0  E0.0208",
//         .ex(30.5f, 26);
//     // "G1 X30.5  Y26.0  E0.832",
// }

// void firstLayer09(gCode &gc) {
//     gc.lastExtrusion(30.5f, 26)
//         .ex(30.5f, 25.5f)
//         // "G1 X30.5  Y25.5  E0.0208",
//         .ex(10.5f, 25.5f)
//         // "G1 X10.5  Y25.5  E0.832",
//         .ex(10.5f, 25)
//         // "G1 X10.5  Y25.0  E0.0208",
//         .ex(30.5f, 25)
//         // "G1 X30.5  Y25.0  E0.832",
//         .ex(30.5f, 24.5f)
//         // "G1 X30.5  Y24.5  E0.0208",
//         .ex(10.5f, 24.5f);
//     // "G1 X10.5  Y24.5  E0.832",
// }

// void firstLayer10(gCode &gc) {
//     gc.lastExtrusion(10.5f, 24.5f)
//         .ex(10.5f, 24)
//         // "G1 X10.5  Y24.0  E0.0208",
//         .ex(30.5f, 24)
//         // "G1 X30.5  Y24.0  E0.832",
//         .ex(30.5f, 23.5f)
//         // "G1 X30.5  Y23.5  E0.0208",
//         .ex(10.5f, 23.5f)
//         // "G1 X10.5  Y23.5  E0.832",
//         .ex(10.5f, 23)
//         // "G1 X10.5  Y23.0  E0.0208",
//         .ex(30.5f, 23);
//     // "G1 X30.5  Y23.0  E0.832",
// }

// void firstLayer11(gCode &gc) {
//     gc.lastExtrusion(30.5f, 23)
//         .ex(30.5f, 22.5f)
//         // "G1 X30.5  Y22.5  E0.0208",
//         .ex(10.5f, 22.5f)
//         // "G1 X10.5  Y22.5  E0.832",
//         .ex(10.5f, 22)
//         // "G1 X10.5  Y22.0  E0.0208",
//         .ex(30.5f, 22)
//         // "G1 X30.5  Y22.0  E0.832",
//         .ex(30.5f, 21.5f)
//         // "G1 X30.5  Y21.5  E0.0208",
//         .ex(10.5f, 21.5f);
//     // "G1 X10.5  Y21.5  E0.832",
// }

// void firstLayer12(gCode &gc) {
//     gc.lastExtrusion(10.5f, 21.5f)
//         .ex(10.5f, 21)
//         // "G1 X10.5  Y21.0  E0.0208",
//         .ex(30.5f, 21)
//         // "G1 X30.5  Y21.0  E0.832",
//         .ex(30.5f, 20.5f)
//         // "G1 X30.5  Y20.5  E0.0208",
//         .ex(10.5f, 20.5f)
//         // "G1 X10.5  Y20.5  E0.832",
//         .ex(10.5f, 20)
//         // "G1 X10.5  Y20.0  E0.0208",
//         .ex(30.5f, 20);
//     // "G1 X30.5  Y20.0  E0.832",
// }

// void firstLayer13(gCode &gc) {
//     gc.lastExtrusion(30.5f, 20)
//         .ex(30.5f, 19.5f)
//         // "G1 X30.5  Y19.5  E0.0208",
//         .ex(10.5f, 19.5f)
//         // "G1 X10.5  Y19.5  E0.832",
//         .ex(10.5f, 19)
//         // "G1 X10.5  Y19.0  E0.0208",
//         .ex(30.5f, 19)
//         // "G1 X30.5  Y19.0  E0.832",
//         .ex(30.5f, 18.5f)
//         // "G1 X30.5  Y18.5  E0.0208",
//         .ex(10.5f, 18.5f);
//     // "G1 X10.5  Y18.5  E0.832",
// }

// void firstLayer14(gCode &gc) {
//     gc.lastExtrusion(10.5f, 18.5f)
//         .ex(10.5f, 18)
//         // "G1 X10.5  Y18.0  E0.0208",
//         .ex(30.5f, 18)
//         // "G1 X30.5  Y18.0  E0.832",
//         .ex(30.5f, 17.5f)
//         // "G1 X30.5  Y17.5  E0.0208",
//         .G1(NAN, NAN, 2, -6, 2100)
//         // "G1 Z2 E-6 F2100",
//         .G1(178, 0, 10, NAN, 3000)
//         // "G1 X178 Y0 Z10 F3000",
//         .G(4);
//     // "G4",
// }

// void firstLayer15(gCode &gc) {
//     gc.M(107)
//         // "M107",
//         .M(104)
//         .param('S', 0)
//         // "M104 S0", // turn off temperature
//         .M(140)
//         .param('S', 0)
//         // "M140 S0", // turn off heatbed
//         .M(84);
//     // "M84"      // disable motors
// }

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

//EXTRUDE_PER_MM  0.2 * 0.5 / (pi * 1.75 ^ 2 / 4) = 0.041575
//todo generate me
// const char *V2_gcodes_body[] = {
//     "G1 Z4 F1000",
//     "G1 X0 Y-2 Z0.2 F3000.0",
//     "G1 E6 F2000",
//     "G1 X60 E9 F1000.0",
//     "G1 X100 E12.5 F1000.0",
//     "G1 Z2 E-6 F2100.00000",

//     "G1 X10 Y150 Z0.2 F3000",
//     "G1 E6 F2000"

//     "G1 F1000",
//     //E = extrusion_length * layer_height * extrusion_width / (PI * pow(1.75, 2) / 4)
//     "G1 X170 Y150 E5.322", //160 * 0.2 * 0.4 / (pi * 1.75 ^ 2 / 4) = 5.322
//     "G1 X170 Y130 E0.665", //20 * 0.2 * 0.4 / (pi * 1.75 ^ 2 / 4) = 0.665
//     "G1 X10  Y130 E5.322",
//     "G1 X10  Y110 E0.665",
//     "G1 X170 Y110 E5.322",
//     "G1 X170 Y90  E0.665",
//     "G1 X10  Y90  E5.322",
//     "G1 X10  Y70  E0.665",
//     "G1 X170 Y70  E5.322",
//     "G1 X170 Y50  E0.665",
//     "G1 X10  Y50  E5.322",

//     //frame around
//     "G1 X10    Y17    E1.371975",  //33 * 0.041575 = 1.371975
//     "G1 X31    Y17    E1.288825",  //31 * 0.041575 = 1.288825
//     "G1 X31    Y30.5  E0.5612625", //13.5 * 0.041575 = 0.5612625
//     "G1 X10.5  Y30.5  E0.832",     //20 * 0.2 * 0.5 / (pi * 1.75 ^ 2 / 4) = 0.832
//     "G1 X10.5  Y30.0  E0.0208",    //0.5 * 0.2 * 0.5 / (pi * 1.75 ^ 2 / 4) = 0.0208

//     "G1 F1000",
//     "G1 X30.5  Y30.0  E0.832",
//     "G1 X30.5  Y29.5  E0.0208",
//     "G1 X10.5  Y29.5  E0.832",
//     "G1 X10.5  Y29.0  E0.0208",
//     "G1 X30.5  Y29.0  E0.832",
//     "G1 X30.5  Y28.5  E0.0208",
//     "G1 X10.5  Y28.5  E0.832",
//     "G1 X10.5  Y28.0  E0.0208",
//     "G1 X30.5  Y28.0  E0.832",
//     "G1 X30.5  Y27.5  E0.0208",
//     "G1 X10.5  Y27.5  E0.832",
//     "G1 X10.5  Y27.0  E0.0208",
//     "G1 X30.5  Y27.0  E0.832",
//     "G1 X30.5  Y26.5  E0.0208",
//     "G1 X10.5  Y26.5  E0.832",
//     "G1 X10.5  Y26.0  E0.0208",
//     "G1 X30.5  Y26.0  E0.832",
//     "G1 X30.5  Y25.5  E0.0208",
//     "G1 X10.5  Y25.5  E0.832",
//     "G1 X10.5  Y25.0  E0.0208",
//     "G1 X30.5  Y25.0  E0.832",
//     "G1 X30.5  Y24.5  E0.0208",
//     "G1 X10.5  Y24.5  E0.832",
//     "G1 X10.5  Y24.0  E0.0208",
//     "G1 X30.5  Y24.0  E0.832",
//     "G1 X30.5  Y23.5  E0.0208",
//     "G1 X10.5  Y23.5  E0.832",
//     "G1 X10.5  Y23.0  E0.0208",
//     "G1 X30.5  Y23.0  E0.832",
//     "G1 X30.5  Y22.5  E0.0208",
//     "G1 X10.5  Y22.5  E0.832",
//     "G1 X10.5  Y22.0  E0.0208",
//     "G1 X30.5  Y22.0  E0.832",
//     "G1 X30.5  Y21.5  E0.0208",
//     "G1 X10.5  Y21.5  E0.832",
//     "G1 X10.5  Y21.0  E0.0208",
//     "G1 X30.5  Y21.0  E0.832",
//     "G1 X30.5  Y20.5  E0.0208",
//     "G1 X10.5  Y20.5  E0.832",
//     "G1 X10.5  Y20.0  E0.0208",
//     "G1 X30.5  Y20.0  E0.832",
//     "G1 X30.5  Y19.5  E0.0208",
//     "G1 X10.5  Y19.5  E0.832",
//     "G1 X10.5  Y19.0  E0.0208",
//     "G1 X30.5  Y19.0  E0.832",
//     "G1 X30.5  Y18.5  E0.0208",
//     "G1 X10.5  Y18.5  E0.832",
//     "G1 X10.5  Y18.0  E0.0208",
//     "G1 X30.5  Y18.0  E0.832",
//     "G1 X30.5  Y17.5  E0.0208",

//     "G1 Z2 E-6 F2100",
//     "G1 X178 Y0 Z10 F3000",

//     "G4",

//     "M107",
//     "M104 S0", // turn off temperature
//     "M140 S0", // turn off heatbed
//     "M84"      // disable motors
// };

//const size_t V2_gcodes_body_sz = 0; //sizeof(V2_gcodes_body) / sizeof(V2_gcodes_body[0]);

//todo use marlin api
// const size_t commands_in_queue_size = 8;
// const size_t commands_in_queue_use_max = 6;
// const size_t max_gcodes_in_one_run = 20; //milion of small gcodes could be done instantly but block gui

//static uint32_t line_head = 0;
//static uint32_t line_body = 0;

// static const char **head_gcode = NULL;
// static const char **body_gcode = NULL;
// static size_t head_gcode_sz = 0; // depreciated
// static size_t body_gcode_sz = -1;
// static size_t gcode_sz = -1;
// static size_t G28_pos = -1;
// static size_t G29_pos = -1;

int _get_progress() {
    //if ( _is_gcode_end_line() ) return 100;
    //return std::min(99, int(100 * (line_head + 1 + line_body + 1) / gcode_sz));
    return std::min(99, int(100.0f * snakeLine / (float)snakeLines));
}

//void _set_gcode_first_lines();

//returns remaining lines
#if DEBUG_TERM == 0
static const char *_wizard_firstlay_text = N_("Once the printer   \n"
                                              "starts extruding   \n"
                                              "plastic, adjust    \n"
                                              "the nozzle height  \n"
                                              "by turning the knob\n"
                                              "until the filament \n"
                                              "sticks to the print\n"
                                              "sheet.");
//int _run_gcode_line(uint32_t *p_line, const char *gcodes[], size_t gcodes_count);
#else
//int _run_gcode_line(uint32_t *p_line, const char *gcodes[], size_t gcodes_count, window_term_t *term);
#endif

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

// inline void FLGcodeMBL(firstlay_screen_t *p_screen) {
// }

inline void FLGcodeHeat(firstlay_screen_t *p_screen) {
    if (marlin_get_gqueue() > 0)
        return;

    heatNozzle(targetTemp());
    p_screen->state = _FL_GCODE_SNAKE_INIT_1;

    p_screen->Z_offset_request = 0; //ignore Z_offset_request variable changes until now
    p_screen->spin_baby_step.color_text = COLOR_ORANGE;
    p_screen->spin_baby_step.Invalidate();

    // #if DEBUG_TERM == 0
    //     const int remaining_lines = _run_gcode_line(&line_head, code, size);
    // #else
    //     const int remaining_lines = _run_gcode_line(&line_head, code, size, &p_screen->term);
    // #endif
    //     if (remaining_lines < 1) {
    //         p_screen->state = _FL_GCODE_BODY;
    // #if DEBUG_TERM == 1
    //         term_printf(&p_screen->terminal, "BODY\n");
    //         p_screen->term.Invalidate();
    // #endif
    //     }
}

// inline void FLGcodeHeadEnd(firstlay_screen_t *p_screen) {
//     if (marlin_get_gqueue() > 0)
//         return;

//     //    p_screen->state = _FL_GCODE_BODY;
//     p_screen->state = _FL_GCODE_SNAKE_INIT_1;
// }

// inline void FLGcodeHead(firstlay_screen_t *p_screen, const char **code, size_t size) {
// #if DEBUG_TERM == 0
//     const int remaining_lines = _run_gcode_line(&line_head, code, size);
// #else
//     const int remaining_lines = _run_gcode_line(&line_head, code, size, &p_screen->term);
// #endif
//     if (remaining_lines < 1) {
//         p_screen->state = _FL_GCODE_BODY;
// #if DEBUG_TERM == 1
//         term_printf(&p_screen->terminal, "BODY\n");
//         p_screen->term.Invalidate();
// #endif
//         p_screen->Z_offset_request = 0; //ignore Z_offset_request variable changes until now
//         p_screen->spin_baby_step.color_text = COLOR_ORANGE;
//         p_screen->spin_baby_step.Invalidate();
//     }
// }

// inline void FLGcodeBody(firstlay_screen_t *p_screen, const char **code, size_t size) {
//     _wizard_firstlay_Z_step(p_screen);
// #if DEBUG_TERM == 0
//     const int remaining_lines = _run_gcode_line(&line_body, code, size);
// #else
//     const int remaining_lines = _run_gcode_line(&line_body, code, size, &p_screen->term);
// #endif
//     if (remaining_lines < 1) {
//         p_screen->state = _FL_GCODE_DONE;
//     }
// }

int wizard_firstlay_print(int16_t id_body, firstlay_screen_t *p_screen, firstlay_data_t *p_data, float z_offset) {
    if (p_data->state_print == _TEST_START) {
        p_screen->state = _FL_INIT;
        p_data->state_print = _TEST_RUN;

        //body_gcode = V2_gcodes_body;
        //body_gcode_sz = V2_gcodes_body_sz;

        //gcode_sz = body_gcode_sz + head_gcode_sz;
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

        // case _FL_GCODE_HEADEND:
        //     if (marlin_get_gqueue() > 0)
        //         return;

        //     //    p_screen->state = _FL_GCODE_BODY;
        //     p_screen->state = _FL_GCODE_SNAKE_INIT_1;
        //     break;

        // case _FL_GCODE_HEAD:
        //     //     //have to wait to next state after MBL to check error
        //     //     if (line_head > G29_pos && marlin_error(MARLIN_ERR_ProbingFailed)) {
        //     //         marlin_error_clr(MARLIN_ERR_ProbingFailed);
        //     //         marlin_gcode("G0 Z30"); //Z 30mm
        //     //         marlin_gcode("M84");    //Disable steppers
        //     //         if (wizard_msgbox(_("Mesh bed leveling failed?"), MSGBOX_BTN_RETRYCANCEL, 0) == MSGBOX_RES_RETRY) {
        //     //             //RETRY
        //     //             line_head = G28_pos;
        //     //         } else {
        //     //             //CANCEL
        //     //             p_data->state_print = _TEST_FAILED;
        //     //             return 100;
        //     //         }
        //     //     }

        //     //     FLGcodeHead(p_screen, head_gcode, head_gcode_sz);
        //     break;

    case _FL_GCODE_SNAKE_INIT_1:
        if (marlin_get_gqueue() > 0)
            break;

        _wizard_firstlay_Z_step(p_screen);
        snakeInit1();
        p_screen->state = _FL_GCODE_SNAKE_INIT_2;
        break;

    case _FL_GCODE_SNAKE_INIT_2:
        if (marlin_get_gqueue() > 0)
            break;

        _wizard_firstlay_Z_step(p_screen);
        snakeInit2();
        p_screen->state = _FL_GCODE_SNAKE_BODY;
        break;

    case _FL_GCODE_SNAKE_BODY:
        if (marlin_get_gqueue() > 0)
            break;

        _wizard_firstlay_Z_step(p_screen);
        sendSnakeLine(snakeLine++);
        if (snakeLine >= snakeLines)
            p_screen->state = _FL_GCODE_SNAKE_END;
        break;

    case _FL_GCODE_SNAKE_END:
        if (marlin_get_gqueue() > 0)
            break;

        _wizard_firstlay_Z_step(p_screen);
        snakeEnd();
        p_screen->state = _FL_GCODE_DONE;
        break;

        // case _FL_GCODE_BODY_01: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     _wizard_firstlay_Z_step(p_screen);
        //     gCode gc;
        //     firstLayer01(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_02;
        //     break;
        // }
        // case _FL_GCODE_BODY_02: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     gCode gc;
        //     firstLayer02(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_03;
        //     break;
        // }
        // case _FL_GCODE_BODY_03: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     gCode gc;
        //     firstLayer03(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_04;
        //     break;
        // }
        // case _FL_GCODE_BODY_04: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     gCode gc;
        //     firstLayer04(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_05;
        //     break;
        // }
        // case _FL_GCODE_BODY_05: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     gCode gc;
        //     firstLayer05(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_06;
        //     break;
        // }
        // case _FL_GCODE_BODY_06: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     gCode gc;
        //     firstLayer06(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_07;
        //     break;
        // }
        // case _FL_GCODE_BODY_07: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     gCode gc;
        //     firstLayer07(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_08;
        //     break;
        // }
        // case _FL_GCODE_BODY_08: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     gCode gc;
        //     firstLayer08(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_09;
        //     break;
        // }
        // case _FL_GCODE_BODY_09: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     gCode gc;
        //     firstLayer09(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_10;
        //     break;
        // }
        // case _FL_GCODE_BODY_10: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     gCode gc;
        //     firstLayer10(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_11;
        //     break;
        // }
        // case _FL_GCODE_BODY_11: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     gCode gc;
        //     firstLayer11(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_12;
        //     break;
        // }
        // case _FL_GCODE_BODY_12: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     gCode gc;
        //     firstLayer12(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_13;
        //     break;
        // }
        // case _FL_GCODE_BODY_13: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     gCode gc;
        //     firstLayer13(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_14;
        //     break;
        // }
        // case _FL_GCODE_BODY_14: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     gCode gc;
        //     firstLayer14(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_15;
        //     break;
        // }
        // case _FL_GCODE_BODY_15: {
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     gCode gc;
        //     firstLayer15(gc);
        //     gc.send();
        //     p_screen->state = _FL_GCODE_BODY_END;
        //     break;
        // }
        // case _FL_GCODE_BODY_END:
        //     if (marlin_get_gqueue() > 0)
        //         break;
        //     p_screen->state = _FL_GCODE_DONE;
        //     break;

        // case _FL_GCODE_BODY:
        //     FLGcodeBody(p_screen, body_gcode, body_gcode_sz);
        //     break;

    case _FL_GCODE_DONE:
        if (marlin_get_gqueue() > 0)
            break;

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

// //returns progress
// void _set_gcode_first_lines() {
//     line_head = 0;
//     line_body = 0;
// }

// #if DEBUG_TERM == 0
// int _run_gcode_line(uint32_t *p_line, const char *gcodes[], size_t gcodes_count)
// #else
// int _run_gcode_line(uint32_t *p_line, const char *gcodes[], size_t gcodes_count, window_term_t *term)
// #endif
// {
//     size_t gcodes_in_this_run = 0;

//     //todo "while" does not work ...why?, something with  commands_in_queue?
//     //while(commands_in_queue < commands_in_queue_use_max){
//     if (marlin_get_gqueue() < 1) {
//         if ((*p_line) < gcodes_count) {
//             ++gcodes_in_this_run;
//             marlin_gcode(gcodes[*p_line]);
// #if DEBUG_TERM == 1
//             term_printf(term->term, "%s\n", gcodes[*p_line]);
//             term->win.Invalidate();
// #endif
//             ++(*p_line);
//         }
//     }

//     //commands_in_queue does not reflect commands added in this run
//     return gcodes_count - (*p_line) + marlin_get_gqueue() + gcodes_in_this_run;
// }
