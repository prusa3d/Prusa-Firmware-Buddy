#include "MindaRedscreen.h"

#include "stm32f4xx_hal.h"
#include "config.h"
#include "gui.h"
#include "st7789v.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "safe_state.h"
#include <inttypes.h>
#include "jogwheel.h"
#include "gpio.h"
#include "sys.h"
#include "hwio.h" //hwio_beeper_set_pwm
#include "wdt.h"
#include "../lang/i18n.h"

#define PADDING 10
#define X_MAX   (display::GetW() - PADDING * 2)

//! @brief Put HW into safe state, activate display safe mode and initialize it twice
static void stop_common(void) {
    hwio_safe_state();
    st7789v_enable_safe_mode();
    hwio_beeper_set_pwm(0, 0);
    display::Init();
    display::Init();
}

#define POINT_CNT           16
#define POINT_PX            7
#define POINT_DIST          40
#define POINT_BASE_X_OFFSET 30
#define POINT_BASE_Y_OFFSET 60
//x y w h
static const rect_ui16_t rct_points[POINT_CNT] = {
    { 3 * POINT_DIST, 3 * POINT_DIST, POINT_PX, POINT_PX }, //bot right point
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
    { 0 * POINT_DIST, 0 * POINT_DIST, POINT_PX, POINT_PX }, //top left point
    { 1 * POINT_DIST, 0 * POINT_DIST, POINT_PX, POINT_PX },
    { 2 * POINT_DIST, 0 * POINT_DIST, POINT_PX, POINT_PX },
    { 3 * POINT_DIST, 0 * POINT_DIST, POINT_PX, POINT_PX }
};

#define MOVE_CNT (POINT_CNT - 1)

#define MOVE_W_PX            5
#define MOVE_DIST_FROM_POINT 3
#define MOVE_L_PX            (POINT_DIST - POINT_PX - 2 * MOVE_DIST_FROM_POINT)

#define MOVE_X_HORIZ (MOVE_DIST_FROM_POINT + POINT_PX)
#define MOVE_Y_HORIZ ((POINT_PX - MOVE_W_PX) / 2)
#define MOVE_X_VERTI MOVE_Y_HORIZ
#define MOVE_Y_VERTI MOVE_X_HORIZ

static const rect_ui16_t rct_moves[POINT_CNT] = {
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

#define BED_EDGE 5
#define BED_W    (3 * POINT_DIST + POINT_PX + 2 * BED_EDGE)
#define BED_H    BED_W

#define BED_TOP_W ((int)(0.47F * (float)BED_W))
#define BED_TOP_H ((int)(0.02F * (float)BED_H))

#define BED_BOT_W ((int)(0.17F * (float)BED_W))
#define BED_BOT_H ((int)(0.04F * (float)BED_H))

void mbl_error(uint16_t moves, uint16_t points) {
    __disable_irq();
    stop_common();
    display::Clear(COLOR_RED_ALERT);

    display::DrawText(rect_ui16(PADDING, PADDING, X_MAX, 22), "MBL ERROR", gui_defaults.font, COLOR_RED_ALERT, COLOR_WHITE);
    display::DrawLine(point_ui16(PADDING, 30), point_ui16(display::GetW() - 1 - PADDING, 30), COLOR_WHITE);

    //bed
    rect_ui16_t rect;
    rect.x = POINT_BASE_X_OFFSET - BED_EDGE;
    rect.y = POINT_BASE_Y_OFFSET - BED_EDGE;
    rect.w = BED_W;
    rect.h = BED_H;

    display::FillRect(rect, COLOR_DARK_KHAKI);
    display::DrawRect(rect, COLOR_BLACK);

    //top part surounding
    rect.x += BED_W / 2 - BED_TOP_W / 2;
    rect.y -= BED_TOP_H;
    rect.w = BED_TOP_W;
    rect.h = BED_TOP_H;

    display::DrawRect(rect, COLOR_BLACK);
    //top part filling
    ++rect.x;
    ++rect.y;
    rect.w -= 2;
    //h must remain the same
    display::FillRect(rect, COLOR_DARK_KHAKI);

    //bot left part surounding
    rect.x = POINT_BASE_X_OFFSET - BED_EDGE;
    rect.y = POINT_BASE_Y_OFFSET - BED_EDGE + BED_H;
    rect.w = BED_BOT_W;
    rect.h = BED_BOT_H;

    display::DrawRect(rect, COLOR_BLACK);
    //bot left part filling
    ++rect.x;
    --rect.y;
    rect.w -= 2;
    //h must remain the same
    display::FillRect(rect, COLOR_DARK_KHAKI);

    //bot right part surounding
    rect.x = POINT_BASE_X_OFFSET - BED_EDGE + BED_W - BED_BOT_W;
    rect.y = POINT_BASE_Y_OFFSET - BED_EDGE + BED_H;
    rect.w = BED_BOT_W;
    rect.h = BED_BOT_H;

    display::DrawRect(rect, COLOR_BLACK);
    //bot right part filling
    ++rect.x;
    --rect.y;
    rect.w -= 2;
    //h must remain the same
    display::FillRect(rect, COLOR_DARK_KHAKI);

    //points
    for (size_t i = 0; i < POINT_CNT; ++i) {
        rect_ui16_t rect = rct_points[i];
        rect.x += POINT_BASE_X_OFFSET;
        rect.y += POINT_BASE_Y_OFFSET;
        if (points & (1 << i)) {
            //err
            display::FillRect(rect, COLOR_RED);

        } else {
            //no err
            display::FillRect(rect, COLOR_GREEN);
        }
        display::DrawRect(rect, COLOR_BLACK);
    }
    //moves
    for (size_t i = 0; i < MOVE_CNT; ++i) {
        rect_ui16_t rect = rct_moves[i];
        rect.x += POINT_BASE_X_OFFSET;
        rect.y += POINT_BASE_Y_OFFSET;
        if (moves & (1 << i)) {
            //err
            display::FillRect(rect, COLOR_RED);

        } else {
            //no err
            display::FillRect(rect, COLOR_GREEN);
        }
        display::DrawRect(rect, COLOR_BLACK);
    }

    render_text_align(rect_ui16(PADDING, 260, X_MAX, 30), _("RESET PRINTER"), gui_defaults.font,
        COLOR_WHITE, COLOR_BLACK, padding_ui8(0, 0, 0, 0), ALIGN_CENTER);

    jogwheel_init();
    gui_reset_jogwheel();

    //cannot use jogwheel_signals  (disabled interrupt)
    while (1) {
        wdt_iwdg_refresh();
        if (!gpio_get(jogwheel_config.pinENC))
            sys_reset(); //button press
    }
}
