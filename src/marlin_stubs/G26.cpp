#include <algorithm>

#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/feature/host_actions.h"
#include "../../lib/Marlin/Marlin/src/feature/safety_timer.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../lib/Marlin/Marlin/src/Marlin.h"
#include "marlin_server.hpp"
//#include "marlin_client.hpp"
#include "client_fsm_types.h"
#include "PrusaGcodeSuite.hpp"
#include "G26.hpp"
#include "cmath_ext.h"

static const constexpr float filamentD = 1.75f;
static const constexpr float layerHeight = 0.2f;
static const constexpr float threadWidth = 0.5f;

static const constexpr float pi = 3.1415926535897932384626433832795f;

void FirstLayer::start_printing() {
    current_line = 0;
    isPrinting_ = true;
}

void FirstLayer::finish_printing() {
    total_lines = 1; /// don't set 0 to avoid division by zero
    isPrinting_ = false;
}

void FirstLayer::go_to_destination(const float x, const float y, const float z, const float e, const float f) {
    if (isfinite(x))
        destination[0] = x;
    else
        destination[0] = current_position[0];

    if (isfinite(y))
        destination[1] = y;
    else
        destination[1] = current_position[1];

    if (isfinite(z))
        destination[2] = z;
    else
        destination[2] = current_position[2];

    if (isfinite(e))
        destination[3] = current_position[3] + e;
    else
        destination[3] = current_position[3];

    if (isfinite(f))
        feedrate_mm_s = f / 60.f;

    prepare_move_to_destination();
}

void FirstLayer::inc_progress() {
    current_line++;
    const uint8_t progress = std::min(uint8_t(100), uint8_t(100.f * current_line / (float)total_lines));
    if (last_progress != progress) {
        last_progress = progress;
        set_var_sd_percent_done(progress);
    }

    // const variant8_t var = variant8_i8(100 * current_line / (float)total_lines);
    // marlin_set_var(MARLIN_VAR_SD_PDONE, var);
}

void FirstLayer::go_to_destination_and_wait(const float x, const float y, const float z, const float e, const float f) {
    go_to_destination(x, y, z, e, f);
    wait_for_move();
    inc_progress();
}

constexpr float FirstLayer::extrusion(const float x1, const float y1, const float x2, const float y2, const float layerHeight, const float threadWidth) const {
    const float length = sqrt(SQR(x2 - x1) + SQR(y2 - y1));
    return length * layerHeight * threadWidth / (pi * SQR(filamentD / 2));
}

constexpr float FirstLayer::extrusion_Manhattan(const float *path, const uint32_t position, const float last) const {
    if (position % 2 == 0) {
        const float x = path[position];
        const float y = path[position - 2];
        return extrusion(x, y, last, y, layerHeight, threadWidth);
    } else {
        const float x = path[position - 1];
        const float y = path[position];
        return extrusion(x, y, x, last, layerHeight, threadWidth);
    }
}

void FirstLayer::print_snake(const float *snake, const size_t snake_size, const float speed) {

    /// move to start
    go_to_destination(snake[0], snake[1], 0); // Process X Y Z E F parameters
    float last_x = snake[0];
    float last_y = snake[1];
    /// iterate positions
    size_t i;
    for (i = 2; i < snake_size - 1; i += 2) { /// snake_size-1 because we need 2 items
        go_to_destination_and_wait(snake[i], NAN, NAN, extrusion_Manhattan(snake, i, last_x), speed);
        last_x = snake[i];
        go_to_destination_and_wait(NAN, snake[i + 1], NAN, extrusion_Manhattan(snake, i + 1, last_y), speed);
        last_y = snake[i + 1];
    }
    if (i == snake_size - 1) { /// process last X movement
        go_to_destination(snake[i], NAN, NAN, extrusion_Manhattan(snake, i, last_x), speed);
    }
}

void FirstLayer::print_shape_1() {
    start_printing();
    total_lines = 11 + ARRAY_SIZE(snake1);
    current_line = 0;

    /// print purge line
    go_to_destination_and_wait(NAN, NAN, 4.f, NAN, 1000.f);
    go_to_destination_and_wait(0.f, -2.f, 0.2f, NAN, 3000.f);
    go_to_destination_and_wait(NAN, NAN, NAN, 6.f, 2000.f);
    go_to_destination_and_wait(60.f, NAN, NAN, 9.f, 1000.f);
    go_to_destination_and_wait(100.f, NAN, NAN, 12.5f, 1000.f);
    go_to_destination_and_wait(NAN, NAN, 2.f, -6.f, 2100.f);

    /// go to starting point and de-retract
    go_to_destination_and_wait(10.f, 150.f, 0.2f, NAN, 3000.f);
    go_to_destination_and_wait(NAN, NAN, NAN, 6.f, 2000.f);
    go_to_destination_and_wait(NAN, NAN, NAN, NAN, 1000.f);

    print_snake(snake1, ARRAY_SIZE(snake1), 1000.f);

    /// finish printing
    go_to_destination_and_wait(NAN, NAN, 2.f, -6.f, 2100.f);
    go_to_destination_and_wait(178.f, 0.f, 10.f, NAN, 3000.f);

    finish_printing();
}

void FirstLayer::print_shape_2() {
    start_printing();
    total_lines = 11 + ARRAY_SIZE(snake2);
    current_line = 0;

    /// print purge line
    go_to_destination_and_wait(NAN, NAN, 4.f, NAN, 1000.f);
    go_to_destination_and_wait(0.f, -2.f, 0.2f, NAN, 3000.f);
    go_to_destination_and_wait(NAN, NAN, NAN, 6.f, 2000.f);
    go_to_destination_and_wait(60.f, NAN, NAN, 9.f, 1000.f);
    go_to_destination_and_wait(100.f, NAN, NAN, 12.5f, 1000.f);
    go_to_destination_and_wait(10.f, 30.f, NAN, extrusion(100.0f, -2.f, 10.f, 30.f), 1000.f);

    print_snake(snake2, ARRAY_SIZE(snake2), 1000.f);

    /// finish printing
    go_to_destination_and_wait(NAN, NAN, 2.f, -6.f, 2100.f);
    go_to_destination_and_wait(178.f, 180.f, 10.f, NAN, 3000.f);

    finish_printing();
}

void PrusaGcodeSuite::G26() {
    if (all_axes_known()) { /// checks if axes are calibrated (homed) before
        FirstLayer fl;
        //fl.print_shape_1();
        fl.print_shape_2();
    }
}
