#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "MindaRedscreen.h"
#include "config.h"
#include "gui.hpp"
#include "safe_state.h"
#include "Jogwheel.hpp"
#include "st7789v.hpp"
#include "gpio.h"
#include "sys.h"
#include "hwio.h" //hwio_beeper_set_pwm
#include "wdt.hpp"
#include "i18n.h"

static const constexpr uint8_t PADDING = 10;
static const constexpr uint16_t X_MAX = display::GetW() - PADDING * 2;

//! @brief Put HW into safe state, activate display safe mode and initialize it twice
static void stop_common(void) {
    hwio_safe_state();
    st7789v_enable_safe_mode();
    hwio_beeper_set_pwm(0, 0);
    display::Init();
    display::Init();
}

static const constexpr uint8_t POINT_CNT = 16;
static const constexpr uint8_t POINT_PX = 7;
static const constexpr uint8_t POINT_DIST = 40;
static const constexpr uint8_t POINT_BASE_X_OFFSET = 30;
static const constexpr uint8_t POINT_BASE_Y_OFFSET = 60;
// x y w h
static const constexpr Rect16 rct_points[POINT_CNT] = {
    { 3 * POINT_DIST, 3 * POINT_DIST, POINT_PX, POINT_PX }, // bot right point
    { 2 * POINT_DIST, 3 * POINT_DIST, POINT_PX, POINT_PX },
    { 1 * POINT_DIST, 3 * POINT_DIST, POINT_PX, POINT_PX },
    { 0 * POINT_DIST, 3 * POINT_DIST, POINT_PX, POINT_PX },
    { 0 * POINT_DIST, 2 * POINT_DIST, POINT_PX, POINT_PX },
    { 1 * POINT_DIST, 2 * POINT_DIST, POINT_PX, POINT_PX },
    { 2 * POINT_DIST, 2 * POINT_DIST, POINT_PX, POINT_PX },
    { 3 * POINT_DIST, 2 * POINT_DIST, POINT_PX, POINT_PX },
    { 3 * POINT_DIST, 1 * POINT_DIST, POINT_PX, POINT_PX },
    { 2 * POINT_DIST, 1 * POINT_DIST, POINT_PX, POINT_PX },
    { 1 * POINT_DIST, 1 * POINT_DIST, POINT_PX, POINT_PX },
    { 0 * POINT_DIST, 1 * POINT_DIST, POINT_PX, POINT_PX },
    { 0 * POINT_DIST, 0 * POINT_DIST, POINT_PX, POINT_PX }, // top left point
    { 1 * POINT_DIST, 0 * POINT_DIST, POINT_PX, POINT_PX },
    { 2 * POINT_DIST, 0 * POINT_DIST, POINT_PX, POINT_PX },
    { 3 * POINT_DIST, 0 * POINT_DIST, POINT_PX, POINT_PX }
};

static const constexpr uint8_t MOVE_CNT = POINT_CNT - 1;

static const constexpr uint8_t MOVE_W_PX = 5;
static const constexpr uint8_t MOVE_DIST_FROM_POINT = 3;
static const constexpr int16_t MOVE_L_PX = POINT_DIST - POINT_PX - 2 * MOVE_DIST_FROM_POINT;

static const constexpr uint8_t MOVE_X_HORIZ = MOVE_DIST_FROM_POINT + POINT_PX;
static const constexpr uint8_t MOVE_Y_HORIZ = (POINT_PX - MOVE_W_PX) / 2;
static const constexpr uint8_t MOVE_X_VERTI = MOVE_Y_HORIZ;
static const constexpr uint8_t MOVE_Y_VERTI = MOVE_X_HORIZ;

static const constexpr Rect16 rct_moves[POINT_CNT] = {
    { MOVE_X_HORIZ + 2 * POINT_DIST, MOVE_Y_HORIZ + 3 * POINT_DIST, MOVE_L_PX, MOVE_W_PX },
    { MOVE_X_HORIZ + 1 * POINT_DIST, MOVE_Y_HORIZ + 3 * POINT_DIST, MOVE_L_PX, MOVE_W_PX },
    { MOVE_X_HORIZ + 0 * POINT_DIST, MOVE_Y_HORIZ + 3 * POINT_DIST, MOVE_L_PX, MOVE_W_PX },
    { MOVE_X_VERTI + 0 * POINT_DIST, MOVE_Y_VERTI + 2 * POINT_DIST, MOVE_W_PX, MOVE_L_PX },
    { MOVE_X_HORIZ + 0 * POINT_DIST, MOVE_Y_HORIZ + 2 * POINT_DIST, MOVE_L_PX, MOVE_W_PX },
    { MOVE_X_HORIZ + 1 * POINT_DIST, MOVE_Y_HORIZ + 2 * POINT_DIST, MOVE_L_PX, MOVE_W_PX },
    { MOVE_X_HORIZ + 2 * POINT_DIST, MOVE_Y_HORIZ + 2 * POINT_DIST, MOVE_L_PX, MOVE_W_PX },
    { MOVE_X_VERTI + 3 * POINT_DIST, MOVE_Y_VERTI + 1 * POINT_DIST, MOVE_W_PX, MOVE_L_PX },
    { MOVE_X_HORIZ + 2 * POINT_DIST, MOVE_Y_HORIZ + 1 * POINT_DIST, MOVE_L_PX, MOVE_W_PX },
    { MOVE_X_HORIZ + 1 * POINT_DIST, MOVE_Y_HORIZ + 1 * POINT_DIST, MOVE_L_PX, MOVE_W_PX },
    { MOVE_X_HORIZ + 0 * POINT_DIST, MOVE_Y_HORIZ + 1 * POINT_DIST, MOVE_L_PX, MOVE_W_PX },
    { MOVE_X_VERTI + 0 * POINT_DIST, MOVE_Y_VERTI + 0 * POINT_DIST, MOVE_W_PX, MOVE_L_PX },
    { MOVE_X_HORIZ + 0 * POINT_DIST, MOVE_Y_HORIZ + 0 * POINT_DIST, MOVE_L_PX, MOVE_W_PX },
    { MOVE_X_HORIZ + 1 * POINT_DIST, MOVE_Y_HORIZ + 0 * POINT_DIST, MOVE_L_PX, MOVE_W_PX },
    { MOVE_X_HORIZ + 2 * POINT_DIST, MOVE_Y_HORIZ + 0 * POINT_DIST, MOVE_L_PX, MOVE_W_PX },
};

static const constexpr uint8_t BED_EDGE = 5;
static const constexpr uint8_t BED_W = 3 * POINT_DIST + POINT_PX + 2 * BED_EDGE;
static const constexpr uint8_t BED_H = BED_W;

static const constexpr int32_t BED_TOP_W = (int)(0.47F * BED_W);
static const constexpr int32_t BED_TOP_H = (int)(0.02F * BED_H);
static const constexpr int32_t BED_BOT_W = (int)(0.17F * BED_W);
static const constexpr int32_t BED_BOT_H = (int)(0.04F * BED_H);

void mbl_error(uint16_t moves, uint16_t points) {
    __disable_irq();
    stop_common();
    display::Clear(COLOR_RED_ALERT);

    static const char mblerr[] = "MBL ERROR";
    display::DrawText(Rect16(PADDING, PADDING, X_MAX, 22), string_view_utf8::MakeCPUFLASH((const uint8_t *)mblerr),
        resource_font(GuiDefaults::DefaultFont), COLOR_RED_ALERT, COLOR_WHITE);
    display::DrawLine(point_ui16(PADDING, 30), point_ui16(display::GetW() - 1 - PADDING, 30), COLOR_WHITE);

    // bed
    Rect16 rect(POINT_BASE_X_OFFSET - BED_EDGE, POINT_BASE_Y_OFFSET - BED_EDGE, BED_W, BED_H);

    display::FillRect(rect, COLOR_DARK_KHAKI);
    display::DrawRect(rect, COLOR_BLACK);

    // top part surounding
    rect = Rect16(rect.Left() + BED_W / 2 - BED_TOP_W / 2, rect.Top() - BED_TOP_H, BED_TOP_W, BED_TOP_H);

    display::DrawRect(rect, COLOR_BLACK);
    // top part filling
    rect += Rect16::Left_t(1);
    rect += Rect16::Top_t(1);
    rect -= Rect16::Width_t(2);
    // h must remain the same
    display::FillRect(rect, COLOR_DARK_KHAKI);

    // bot left part surounding
    rect = Rect16(POINT_BASE_X_OFFSET - BED_EDGE, POINT_BASE_Y_OFFSET - BED_EDGE + BED_H, BED_BOT_W, BED_BOT_H);

    display::DrawRect(rect, COLOR_BLACK);
    // bot left part filling
    rect += Rect16::Left_t(1);
    rect -= Rect16::Top_t(1);
    rect -= Rect16::Width_t(2);
    // h must remain the same
    display::FillRect(rect, COLOR_DARK_KHAKI);

    // bot right part surounding
    rect = Rect16(POINT_BASE_X_OFFSET - BED_EDGE + BED_W - BED_BOT_W, POINT_BASE_Y_OFFSET - BED_EDGE + BED_H, BED_BOT_W, BED_BOT_H);

    display::DrawRect(rect, COLOR_BLACK);
    // bot right part filling
    rect += Rect16::Left_t(1);
    rect -= Rect16::Top_t(1);
    rect -= Rect16::Width_t(2);
    // h must remain the same
    display::FillRect(rect, COLOR_DARK_KHAKI);

    // points
    for (size_t i = 0; i < POINT_CNT; ++i) {
        Rect16 rct = rct_points[i];
        rct += Rect16::Left_t(POINT_BASE_X_OFFSET);
        rct += Rect16::Top_t(POINT_BASE_Y_OFFSET);
        if (points & (1 << i)) {
            // err
            display::FillRect(rct, COLOR_RED);

        } else {
            // no err
            display::FillRect(rct, COLOR_GREEN);
        }
        display::DrawRect(rct, COLOR_BLACK);
    }
    // moves
    for (size_t i = 0; i < MOVE_CNT; ++i) {
        Rect16 rct = rct_moves[i];
        rct += Rect16::Left_t(POINT_BASE_X_OFFSET);
        rct += Rect16::Top_t(POINT_BASE_Y_OFFSET);
        if (moves & (1 << i)) {
            // err
            display::FillRect(rct, COLOR_RED);

        } else {
            // no err
            display::FillRect(rct, COLOR_GREEN);
        }
        display::DrawRect(rct, COLOR_BLACK);
    }

    render_text_align(Rect16(PADDING, 260, X_MAX, 30), _("RESET PRINTER"), GuiDefaults::DefaultFont,
        COLOR_WHITE, COLOR_BLACK, { 0, 0, 0, 0 }, Align_t::Center());

    // cannot use jogwheel_signals  (disabled interrupt)
    while (1) {
        wdt_iwdg_refresh();
        if (!jogwheel.GetJogwheelButtonPinState()) {
            sys_reset(); // button press
        }
    }
}
