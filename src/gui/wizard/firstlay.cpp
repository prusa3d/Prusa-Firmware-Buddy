// firstlay.c

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <math.h>
#include "cmath_ext.h"

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

constexpr uint16_t bufferSize = 1024;
constexpr float layerHeight = 0.2f;
constexpr float threadWidth = 0.5f;
constexpr float filamentD = 1.75f;
constexpr float pi = 3.1415926535897932384626433832795;
constexpr float extrudeCoef = layerHeight * threadWidth / (pi * SQR(filamentD / 2));

/// Tool to generate string from G codes.
/// Most of the methods can be chained (gc.G(28).G(29).G1(0,0,0,0,0))
class gCode {
private:
    /// G code string; stores whole G code
    std::array<char, bufferSize> code;

    /// Cursor position in code.
    /// This defines end of the string.
    /// There's no need for trailing 0.
    uint16_t pos = 0;
    uint8_t error_ = 0;
    float x_ = NAN;
    float y_ = NAN;

    void updatePosOrError(const int chars, const uint8_t errorNum = 1) {
        if (chars < 0) {
            error_ = errorNum;
        } else {
            pos += chars;
        }
    }

    void addNewLineIfMissing() {
        if (pos > 0 && !isFull() && code[pos - 1] != '\n')
            code[pos++] = '\n';
    }

public:
    gCode() {
        code[0] = 0;
    }

    bool isFull() {
        if (pos >= bufferSize)
            return true;
        return false;
    }

    uint8_t error() {
        return error_;
    }

    inline bool isError() {
        return error_ > 0;
    }

    void resetError() {
        error_ = 0;
    }

    void clear() {
        pos = 0;
    }

    /// Writes a string.
    gCode &write(const char *string) {
        if (isError())
            return *this;

        const int chars = snprintf(&code[pos], bufferSize - pos, string);
        updatePosOrError(chars);
        return *this;
    }

    /// Writes a float.
    gCode &write(const float value) {
        if (isError())
            return *this;

        const int chars = snprintf(&code[pos], bufferSize - pos, "%f", (double)value);
        updatePosOrError(chars);
        return *this;
    }

    /// Writes a parameter e.g. " S120"
    gCode &param(const char parameter, const float value) {
        if (isError())
            return *this;

        const int chars = snprintf(&code[pos], bufferSize - pos, " %c%f", parameter, (double)value);
        updatePosOrError(chars);
        return *this;
    }

    /// Writes e.g. "G28\n" or "G28 ".
    gCode &G(const uint8_t value, const bool lineBreak = true) {
        if (isError())
            return *this;

        addNewLineIfMissing();
        const int chars = snprintf(&code[pos], bufferSize - pos, "G%d", value);

        if (chars < 0) {
            error_ = 1;
        } else {
            pos += chars;
            if (lineBreak)
                newLine();
            else
                space();
        }
        return *this;
    }

    /// Writes e.g. "M83\n" or "M83 ".
    gCode &M(const uint8_t value, const bool lineBreak = true) {
        if (isError())
            return *this;

        addNewLineIfMissing();
        const int chars = snprintf(&code[pos], bufferSize - pos, "M%d", value);

        if (chars < 0) {
            error_ = 1;
        } else {
            pos += chars;
            if (lineBreak)
                newLine();
            else
                space();
        }
        return *this;
    }

    /// Add space.
    gCode &space() {
        if (isFull() || isError()) {
            error_ = 2;
        } else {
            code[pos++] = ' ';
        }
        return *this;
    }

    /// Add new line.
    gCode &newLine() {
        if (isFull() || isError()) {
            error_ = 2;
        } else {
            code[pos++] = '\n';
        }
        return *this;
    }

    /// Writes e.g. "G1 X10 Y20 Z30 E0.5 F1500\n".
    gCode &G1(const float x, const float y, const float z, const float e, const float f) {
        if (isError())
            return *this;

        G(1);

        int chars = 0;
        if (isfinite(x)) {
            x_ = x;
            chars = snprintf(&code[pos], bufferSize - pos, " X%f", (double)x);
            if (chars < 0) {
                error_ = 3;
                return *this;
            }
            pos += chars;
        }

        if (isfinite(y)) {
            y_ = y;
            chars = snprintf(&code[pos], bufferSize - pos, " Y%f", (double)y);
            if (chars < 0) {
                error_ = 3;
                return *this;
            }
            pos += chars;
        }

        if (isfinite(z)) {
            chars = snprintf(&code[pos], bufferSize - pos, " Z%f", (double)z);
            if (chars < 0) {
                error_ = 3;
                return *this;
            }
            pos += chars;
        }

        if (isfinite(e)) {
            chars = snprintf(&code[pos], bufferSize - pos, " E%f", (double)e);
            if (chars < 0) {
                error_ = 3;
                return *this;
            }
            pos += chars;
        }

        if (isfinite(f)) {
            chars = snprintf(&code[pos], bufferSize - pos, " F%f", (double)f);
            if (chars < 0) {
                error_ = 3;
                return *this;
            }
            pos += chars;
        }

        chars = snprintf(&code[pos], bufferSize - pos, "\n");
        updatePosOrError(chars, 3);

        return *this;
    }

    /// Writes e.g. "G1 X10 Y20 E0.5".
    gCode &G1(const float x, const float y, const float e) {
        return G1(x, y, NAN, e, NAN);
    }

    /// Sends G codes one by one
    /// \returns false if error occurs
    bool send() {
        if (pos >= bufferSize) {
            error_ = 4;
            return false;
        }

        uint16_t eol = 0;
        uint16_t start = 0;
        while (eol < pos) {
            /// find end of line
            while (eol < pos && code[eol] != '\n')
                ++eol;
            code[eol] = '\0'; /// switch \n to \0
            marlin_gcode(&code[start]);
            start = ++eol;
        }
        clear(); /// remove all G codes
        return true;
    }

    /// Extrude from the last point to the specified one
    /// Extrusion uses pre-defined height and width
    gCode ex(const float x, const float y) {
        if (!isfinite(x_) || !isfinite(y_))
            return;
        const float length = sqrt(SQR(x - x_) + SQR(y - y_));
        G1(x, y, length * extrudeCoef);
    }

    /// Set the last point of extrusion
    /// Does not add anything to the G codes
    gCode lastExtrusion(const float x, const float y) {
        x_ = x;
        y_ = y;
    }
};

void initialGcodes(gCode &gc) {
    gc.M(107)   // fan off
        .G(90)  // use absolute coordinates
        .M(83)  // extruder relative mode
        .G(21)  // set units to millimeters
        .M(83); // use relative distances for extrusion
}

void homeAndMBL(gCode &gc, const uint16_t nozzle_preheat, const uint16_t nozzle_target, const uint8_t bed) {
    // clang-format off
    gc  .M(104, false).param('S', nozzle_preheat).param('D', nozzle_target).newLine() //nozzle target
        .M(140, false).param('S', bed)                                                // bed target
        .M(109, false).param('R', nozzle_preheat).newLine()                           // wait for nozzle temp
        .M(190, false).param('S', bed).newLine()                                      // wait for bed temp
        .G(28)                                                                        // autohome
        .G(29);                                                                       // meshbed leveling
    // clang-format on
}

void heatNozzle(gCode &gc, const uint16_t nozzle_target) {
    // clang-format off
    gc.M(104, false).param('S', nozzle_target).newLine()  // nozzle target
         .M(109, false).param('S', nozzle_target).newLine(); // wait for nozzle temp
    // clang-format on
}

void firstLayer01(gCode &gc) {
    gc.G1(NAN, NAN, 4, NAN, 1000)
        // "G1 Z4 F1000",
        .G1(0, -2, 0.2, NAN, 3000)
        // "G1 X0 Y-2 Z0.2 F3000.0",
        .G1(NAN, NAN, NAN, 6, 2000)
        // "G1 E6 F2000",
        .G1(60, NAN, NAN, 9, 1000)
        // "G1 X60 E9 F1000.0",
        .G1(100, NAN, NAN, 12.5, 1000)
        // "G1 X100 E12.5 F1000.0",
        .G1(NAN, NAN, 2, -6, 2100);
    // "G1 Z2 E-6 F2100.00000",
}

void firstLayer02(gCode &gc) {
    gc.G1(10, 150, 0.2, NAN, 3000)
        // "G1 X10 Y150 Z0.2 F3000",
        .G1(NAN, NAN, NAN, 6, 2000)
        // "G1 E6 F2000"
        .G1(NAN, NAN, NAN, NAN, 1000);
    // "G1 F1000",
}

void firstLayer03(gCode &gc) {
    // //E = extrusion_length * layer_height * extrusion_width / (PI * pow(1.75, 2) / 4)
    gc.lastExtrusion(10, 150)
        .ex(170, 150)
        // "G1 X170 Y150 E5.322", //160 * 0.2 * 0.4 / (pi * 1.75 ^ 2 / 4) = 5.322
        .ex(170, 130)
        // "G1 X170 Y130 E0.665", //20 * 0.2 * 0.4 / (pi * 1.75 ^ 2 / 4) = 0.665
        .ex(10, 130)
        // "G1 X10  Y130 E5.322",
        .ex(10, 110)
        // "G1 X10  Y110 E0.665",
        .ex(170, 110)
        // "G1 X170 Y110 E5.322",
        .ex(170, 90);
    // "G1 X170 Y90  E0.665",
}

void firstLayer04(gCode &gc) {

    // "G1 X10  Y90  E5.322",
    // "G1 X10  Y70  E0.665",
    // "G1 X170 Y70  E5.322",
    // "G1 X170 Y50  E0.665",
    // "G1 X10  Y50  E5.322",
}
void firstLayer05(gCode &gc) {
    // //frame around
    // "G1 X10    Y17    E1.371975",  //33 * 0.041575 = 1.371975
    // "G1 X31    Y17    E1.288825",  //31 * 0.041575 = 1.288825
    // "G1 X31    Y30.5  E0.5612625", //13.5 * 0.041575 = 0.5612625
    // "G1 X10.5  Y30.5  E0.832",     //20 * 0.2 * 0.5 / (pi * 1.75 ^ 2 / 4) = 0.832
    // "G1 X10.5  Y30.0  E0.0208",    //0.5 * 0.2 * 0.5 / (pi * 1.75 ^ 2 / 4) = 0.0208
}
void firstLayer06(gCode &gc) {
    // "G1 F1000",
    // "G1 X30.5  Y30.0  E0.832",
    // "G1 X30.5  Y29.5  E0.0208",
    // "G1 X10.5  Y29.5  E0.832",
    // "G1 X10.5  Y29.0  E0.0208",
    // "G1 X30.5  Y29.0  E0.832",
}
void firstLayer07(gCode &gc) {
    // "G1 X30.5  Y28.5  E0.0208",
    // "G1 X10.5  Y28.5  E0.832",
    // "G1 X10.5  Y28.0  E0.0208",
    // "G1 X30.5  Y28.0  E0.832",
    // "G1 X30.5  Y27.5  E0.0208",
    // "G1 X10.5  Y27.5  E0.832",
}
void firstLayer08(gCode &gc) {
    // "G1 X10.5  Y27.0  E0.0208",
    // "G1 X30.5  Y27.0  E0.832",
    // "G1 X30.5  Y26.5  E0.0208",
    // "G1 X10.5  Y26.5  E0.832",
    // "G1 X10.5  Y26.0  E0.0208",
    // "G1 X30.5  Y26.0  E0.832",
}
void firstLayer09(gCode &gc) {
    // "G1 X30.5  Y25.5  E0.0208",
    // "G1 X10.5  Y25.5  E0.832",
    // "G1 X10.5  Y25.0  E0.0208",
    // "G1 X30.5  Y25.0  E0.832",
    // "G1 X30.5  Y24.5  E0.0208",
    // "G1 X10.5  Y24.5  E0.832",
}
void firstLayer10(gCode &gc) {
    // "G1 X10.5  Y24.0  E0.0208",
    // "G1 X30.5  Y24.0  E0.832",
    // "G1 X30.5  Y23.5  E0.0208",
    // "G1 X10.5  Y23.5  E0.832",
    // "G1 X10.5  Y23.0  E0.0208",
    // "G1 X30.5  Y23.0  E0.832",
}
void firstLayer11(gCode &gc) {
    // "G1 X30.5  Y22.5  E0.0208",
    // "G1 X10.5  Y22.5  E0.832",
    // "G1 X10.5  Y22.0  E0.0208",
    // "G1 X30.5  Y22.0  E0.832",
    // "G1 X30.5  Y21.5  E0.0208",
    // "G1 X10.5  Y21.5  E0.832",
}
void firstLayer12(gCode &gc) {
    // "G1 X10.5  Y21.0  E0.0208",
    // "G1 X30.5  Y21.0  E0.832",
    // "G1 X30.5  Y20.5  E0.0208",
    // "G1 X10.5  Y20.5  E0.832",
    // "G1 X10.5  Y20.0  E0.0208",
    // "G1 X30.5  Y20.0  E0.832",
}
void firstLayer13(gCode &gc) {
    // "G1 X30.5  Y19.5  E0.0208",
    // "G1 X10.5  Y19.5  E0.832",
    // "G1 X10.5  Y19.0  E0.0208",
    // "G1 X30.5  Y19.0  E0.832",
    // "G1 X30.5  Y18.5  E0.0208",
    // "G1 X10.5  Y18.5  E0.832",
}
void firstLayer14(gCode &gc) {
    // "G1 X10.5  Y18.0  E0.0208",
    // "G1 X30.5  Y18.0  E0.832",
    // "G1 X30.5  Y17.5  E0.0208",

    // "G1 Z2 E-6 F2100",
    // "G1 X178 Y0 Z10 F3000",

    // "G4",
}
void firstLayer15(gCode &gc) {
    // "M107",
    // "M104 S0", // turn off temperature
    // "M140 S0", // turn off heatbed
    // "M84"      // disable motors
}

//EXTRUDE_PER_MM  0.2 * 0.5 / (pi * 1.75 ^ 2 / 4) = 0.041575
//todo generate me
const char *V2_gcodes_body[] = {
    "G1 Z4 F1000",
    "G1 X0 Y-2 Z0.2 F3000.0",
    "G1 E6 F2000",
    "G1 X60 E9 F1000.0",
    "G1 X100 E12.5 F1000.0",
    "G1 Z2 E-6 F2100.00000",

    "G1 X10 Y150 Z0.2 F3000",
    "G1 E6 F2000"

    "G1 F1000",
    //E = extrusion_length * layer_height * extrusion_width / (PI * pow(1.75, 2) / 4)
    "G1 X170 Y150 E5.322", //160 * 0.2 * 0.4 / (pi * 1.75 ^ 2 / 4) = 5.322
    "G1 X170 Y130 E0.665", //20 * 0.2 * 0.4 / (pi * 1.75 ^ 2 / 4) = 0.665
    "G1 X10  Y130 E5.322",
    "G1 X10  Y110 E0.665",
    "G1 X170 Y110 E5.322",
    "G1 X170 Y90  E0.665",
    "G1 X10  Y90  E5.322",
    "G1 X10  Y70  E0.665",
    "G1 X170 Y70  E5.322",
    "G1 X170 Y50  E0.665",
    "G1 X10  Y50  E5.322",

    //frame around
    "G1 X10    Y17    E1.371975",  //33 * 0.041575 = 1.371975
    "G1 X31    Y17    E1.288825",  //31 * 0.041575 = 1.288825
    "G1 X31    Y30.5  E0.5612625", //13.5 * 0.041575 = 0.5612625
    "G1 X10.5  Y30.5  E0.832",     //20 * 0.2 * 0.5 / (pi * 1.75 ^ 2 / 4) = 0.832
    "G1 X10.5  Y30.0  E0.0208",    //0.5 * 0.2 * 0.5 / (pi * 1.75 ^ 2 / 4) = 0.0208

    "G1 F1000",
    "G1 X30.5  Y30.0  E0.832",
    "G1 X30.5  Y29.5  E0.0208",
    "G1 X10.5  Y29.5  E0.832",
    "G1 X10.5  Y29.0  E0.0208",
    "G1 X30.5  Y29.0  E0.832",
    "G1 X30.5  Y28.5  E0.0208",
    "G1 X10.5  Y28.5  E0.832",
    "G1 X10.5  Y28.0  E0.0208",
    "G1 X30.5  Y28.0  E0.832",
    "G1 X30.5  Y27.5  E0.0208",
    "G1 X10.5  Y27.5  E0.832",
    "G1 X10.5  Y27.0  E0.0208",
    "G1 X30.5  Y27.0  E0.832",
    "G1 X30.5  Y26.5  E0.0208",
    "G1 X10.5  Y26.5  E0.832",
    "G1 X10.5  Y26.0  E0.0208",
    "G1 X30.5  Y26.0  E0.832",
    "G1 X30.5  Y25.5  E0.0208",
    "G1 X10.5  Y25.5  E0.832",
    "G1 X10.5  Y25.0  E0.0208",
    "G1 X30.5  Y25.0  E0.832",
    "G1 X30.5  Y24.5  E0.0208",
    "G1 X10.5  Y24.5  E0.832",
    "G1 X10.5  Y24.0  E0.0208",
    "G1 X30.5  Y24.0  E0.832",
    "G1 X30.5  Y23.5  E0.0208",
    "G1 X10.5  Y23.5  E0.832",
    "G1 X10.5  Y23.0  E0.0208",
    "G1 X30.5  Y23.0  E0.832",
    "G1 X30.5  Y22.5  E0.0208",
    "G1 X10.5  Y22.5  E0.832",
    "G1 X10.5  Y22.0  E0.0208",
    "G1 X30.5  Y22.0  E0.832",
    "G1 X30.5  Y21.5  E0.0208",
    "G1 X10.5  Y21.5  E0.832",
    "G1 X10.5  Y21.0  E0.0208",
    "G1 X30.5  Y21.0  E0.832",
    "G1 X30.5  Y20.5  E0.0208",
    "G1 X10.5  Y20.5  E0.832",
    "G1 X10.5  Y20.0  E0.0208",
    "G1 X30.5  Y20.0  E0.832",
    "G1 X30.5  Y19.5  E0.0208",
    "G1 X10.5  Y19.5  E0.832",
    "G1 X10.5  Y19.0  E0.0208",
    "G1 X30.5  Y19.0  E0.832",
    "G1 X30.5  Y18.5  E0.0208",
    "G1 X10.5  Y18.5  E0.832",
    "G1 X10.5  Y18.0  E0.0208",
    "G1 X30.5  Y18.0  E0.832",
    "G1 X30.5  Y17.5  E0.0208",

    "G1 Z2 E-6 F2100",
    "G1 X178 Y0 Z10 F3000",

    "G4",

    "M107",
    "M104 S0", // turn off temperature
    "M140 S0", // turn off heatbed
    "M84"      // disable motors
};

const size_t V2_gcodes_body_sz = sizeof(V2_gcodes_body) / sizeof(V2_gcodes_body[0]);

//todo use marlin api
const size_t commands_in_queue_size = 8;
const size_t commands_in_queue_use_max = 6;
const size_t max_gcodes_in_one_run = 20; //milion of small gcodes could be done instantly but block gui

static uint32_t line_head = 0;
static uint32_t line_body = 0;

static const char **head_gcode = NULL;
static const char **body_gcode = NULL;
static size_t head_gcode_sz = 0; // depreciated
static size_t body_gcode_sz = -1;
static size_t gcode_sz = -1;
static size_t G28_pos = -1;
static size_t G29_pos = -1;

int _get_progress();

void _set_gcode_first_lines();

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
int _run_gcode_line(uint32_t *p_line, const char *gcodes[], size_t gcodes_count);
#else
int _run_gcode_line(uint32_t *p_line, const char *gcodes[], size_t gcodes_count, window_term_t *term);
#endif

void _wizard_firstlay_Z_step(firstlay_screen_t *p_screen);

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

float bedTemp() {
    return get_filament_bed_temp();
}

inline void FLInit(int16_t id_body, firstlay_screen_t *p_screen, firstlay_data_t *p_data, float z_offset) {
    p_screen->Z_offset = z_offset;
    wizard_init_screen_firstlay(id_body, p_screen, p_data);

    gCode gc;
    initialGcodes(gc);
    gc.send();

#if DEBUG_TERM == 1
    term_printf(&p_screen->terminal, "INITIALIZED\n");
    p_screen->term.id.Invalidate();
#endif
    _set_gcode_first_lines();
    p_screen->state = _FL_GCODE_MBL;
    marlin_error_clr(MARLIN_ERR_ProbingFailed);
#if DEBUG_TERM == 1
    term_printf(&p_screen->terminal, "HEAD\n");
    p_screen->term.id.Invalidate();
#endif
}

inline void FLGcodeMBL(firstlay_screen_t *p_screen) {
    if (marlin_get_gqueue() > 0)
        return;

    gCode gc;
    homeAndMBL(gc, preheatTemp(), targetTemp(), bedTemp());
    gc.send();
    p_screen->state = _FL_GCODE_HEAT;
}

inline void FLGcodeHeat(firstlay_screen_t *p_screen) {
    if (marlin_get_gqueue() > 0)
        return;

    gCode gc;
    heatNozzle(gc, targetTemp());
    gc.send();
    p_screen->state = _FL_GCODE_HEADEND;

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

inline void FLGcodeHeadEnd(firstlay_screen_t *p_screen) {
    if (marlin_get_gqueue() > 0)
        return;

    p_screen->state = _FL_GCODE_BODY;
}

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

inline void FLGcodeBody(firstlay_screen_t *p_screen, const char **code, size_t size) {
    _wizard_firstlay_Z_step(p_screen);
#if DEBUG_TERM == 0
    const int remaining_lines = _run_gcode_line(&line_body, code, size);
#else
    const int remaining_lines = _run_gcode_line(&line_body, code, size, &p_screen->term);
#endif
    if (remaining_lines < 1) {
        p_screen->state = _FL_GCODE_DONE;
    }
}

int wizard_firstlay_print(int16_t id_body, firstlay_screen_t *p_screen, firstlay_data_t *p_data, float z_offset) {
    if (p_data->state_print == _TEST_START) {
        p_screen->state = _FL_INIT;
        p_data->state_print = _TEST_RUN;

        body_gcode = V2_gcodes_body;
        body_gcode_sz = V2_gcodes_body_sz;

        gcode_sz = body_gcode_sz + head_gcode_sz;
    }

    switch (p_screen->state) {
    case _FL_INIT:
        FLInit(id_body, p_screen, p_data, z_offset);
        break;

    case _FL_GCODE_MBL:
        FLGcodeMBL(p_screen);
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

    case _FL_GCODE_HEADEND:
        FLGcodeHeadEnd(p_screen);
        break;

    case _FL_GCODE_HEAD:
        //     //have to wait to next state after MBL to check error
        //     if (line_head > G29_pos && marlin_error(MARLIN_ERR_ProbingFailed)) {
        //         marlin_error_clr(MARLIN_ERR_ProbingFailed);
        //         marlin_gcode("G0 Z30"); //Z 30mm
        //         marlin_gcode("M84");    //Disable steppers
        //         if (wizard_msgbox(_("Mesh bed leveling failed?"), MSGBOX_BTN_RETRYCANCEL, 0) == MSGBOX_RES_RETRY) {
        //             //RETRY
        //             line_head = G28_pos;
        //         } else {
        //             //CANCEL
        //             p_data->state_print = _TEST_FAILED;
        //             return 100;
        //         }
        //     }

        //     FLGcodeHead(p_screen, head_gcode, head_gcode_sz);
        break;

    case _FL_GCODE_BODY:
        FLGcodeBody(p_screen, body_gcode, body_gcode_sz);
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

int _get_progress() {
    //if ( _is_gcode_end_line() ) return 100;
    return std::min(99, int(100 * (line_head + 1 + line_body + 1) / gcode_sz));
}

//returns progress
void _set_gcode_first_lines() {
    line_head = 0;
    line_body = 0;
}

#if DEBUG_TERM == 0
int _run_gcode_line(uint32_t *p_line, const char *gcodes[], size_t gcodes_count)
#else
int _run_gcode_line(uint32_t *p_line, const char *gcodes[], size_t gcodes_count, window_term_t *term)
#endif
{
    size_t gcodes_in_this_run = 0;

    //todo "while" does not work ...why?, something with  commands_in_queue?
    //while(commands_in_queue < commands_in_queue_use_max){
    if (marlin_get_gqueue() < 1) {
        if ((*p_line) < gcodes_count) {
            ++gcodes_in_this_run;
            marlin_gcode(gcodes[*p_line]);
#if DEBUG_TERM == 1
            term_printf(term->term, "%s\n", gcodes[*p_line]);
            term->win.Invalidate();
#endif
            ++(*p_line);
        }
    }

    //commands_in_queue does not reflect commands added in this run
    return gcodes_count - (*p_line) + marlin_get_gqueue() + gcodes_in_this_run;
}
