/**
 * @file
 */
#include <algorithm>

#include "../../lib/Marlin/Marlin/src/module/temperature.h"
#include "../../lib/Marlin/Marlin/src/gcode/lcd/M73_PE.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "marlin_server.hpp"
#include "PrusaGcodeSuite.hpp"
#include "filament.hpp"
#include "G26.hpp"
#include "cmath_ext.h"
#include <config_store/store_instance.hpp>

namespace {
constexpr float filamentD = 1.75f;
constexpr float layerHeight = 0.2f;
constexpr float threadWidth = 0.5f;

constexpr float pi = 3.1415926535897932384626433832795f;

/// Path of Manhattan snake
/// Alternate X and Y coordinates.
/// It cannot print diagonals etc.
/// First X and Y are a starting point.
constexpr float snake1[] = {
    /// use 0.5 extrusion width
    10, /// start X
    150, /// start Y
    170, /// X
    130, /// Y
    10, /// X
    110, /// ...
    170,
    90,
    10,
    70,
    170,
    50,
    ///
    /// frame around
    9.5,
    17,
    30.5,
    30.5,
    10,
    30,
    ///
    /// infill
    30,
    29.5,
    10,
    29,
    30,
    28.5,
    10,
    28,
    30,
    27.5,
    10,
    27,
    30,
    26.5,
    10,
    26,
    30,
    25.5,
    10,
    25,
    30,
    24.5,
    10,
    24,
    30,
    23.5,
    10,
    23,
    30,
    22.5,
    10,
    22,
    30,
    21.5,
    10,
    21,
    30,
    20.5,
    10,
    20,
    30,
    19.5,
    10,
    19,
    30,
    18.5,
    10,
    18,
    30,
    17.5,
    10,
};

#if PRINTER_IS_PRUSA_MINI
constexpr float y_step = 20;
#else
constexpr float y_step = 30;
#endif
constexpr float x_min = 10;
constexpr float x_max = X_BED_SIZE - 10;
constexpr float y_min = 30;
constexpr float y1 = y_min + y_step;
constexpr float y2 = y1 + y_step;
constexpr float y3 = y2 + y_step;
constexpr float y4 = y3 + y_step;
constexpr float y5 = y4 + y_step;
static_assert(Y_BED_SIZE > y5, "snake pattern out of bed");

/// Path of Manhattan snake
/// Alternate X and Y coordinates.
/// It cannot print diagonals etc.
/// First X and Y are a starting point.
constexpr float snake2[] = {
    /// use 0.5 extrusion width
    x_min, /// start X
    y_min, /// start Y
    x_max, /// X
    y1, /// Y
    x_min, /// X
    y2, /// ...
    x_max,
    y3,
    x_min,
    y4,
    x_max,
    y5,
    ///
    /// frame around
    9.5,
    Y_BED_SIZE - 17,
    30.5,
    Y_BED_SIZE - 30.5,
    x_min,
    Y_BED_SIZE - 30,
    ///
    /// infill
    30,
    Y_BED_SIZE - 29.5,
    x_min,
    Y_BED_SIZE - 29,
    30,
    Y_BED_SIZE - 28.5,
    x_min,
    Y_BED_SIZE - 28,
    30,
    Y_BED_SIZE - 27.5,
    x_min,
    Y_BED_SIZE - 27,
    30,
    Y_BED_SIZE - 26.5,
    x_min,
    Y_BED_SIZE - 26,
    30,
    Y_BED_SIZE - 25.5,
    x_min,
    Y_BED_SIZE - 25,
    30,
    Y_BED_SIZE - 24.5,
    x_min,
    Y_BED_SIZE - 24,
    30,
    Y_BED_SIZE - 23.5,
    x_min,
    Y_BED_SIZE - 23,
    30,
    Y_BED_SIZE - 22.5,
    x_min,
    Y_BED_SIZE - 22,
    30,
    Y_BED_SIZE - 21.5,
    x_min,
    Y_BED_SIZE - 21,
    30,
    Y_BED_SIZE - 20.5,
    x_min,
    Y_BED_SIZE - 20,
    30,
    Y_BED_SIZE - 19.5,
    x_min,
    Y_BED_SIZE - 19,
    30,
    Y_BED_SIZE - 18.5,
    x_min,
    Y_BED_SIZE - 18,
    30,
    Y_BED_SIZE - 17.5,
    x_min,
};
} // anonymous namespace

// static variables
FirstLayer *FirstLayer::instance_ = nullptr;

void FirstLayer::finish_printing() {
    current_line = 1;
    total_lines = 1; /// don't set 0 to avoid division by zero
}

uint8_t FirstLayer::progress_percent() const {
    return (total_lines > 0) ? std::min(uint8_t(100), uint8_t(100.f * current_line / (float)total_lines)) : 0;
}

void FirstLayer::plan_destination(const float x, const float y, const float z, const float e, const float f) {
    if (isfinite(x)) {
        destination[0] = x;
    } else {
        destination[0] = current_position[0];
    }

    if (isfinite(y)) {
        destination[1] = y;
    } else {
        destination[1] = current_position[1];
    }

    if (isfinite(z)) {
        destination[2] = z;
    } else {
        destination[2] = current_position[2];
    }

    if (isfinite(e)) {
        destination[3] = current_position[3] + e;
    } else {
        destination[3] = current_position[3];
    }

    if (isfinite(f)) {
        feedrate_mm_s = f / 60.f;
    }

    prepare_move_to_destination();
}

void FirstLayer::go_to_destination(const float x, const float y, const float z, const float e, const float f) {
    plan_destination(x, y, z, e, f);
    wait_for_move();
    current_line++;
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
    plan_destination(snake[0], snake[1], 0); // Process X Y Z E F parameters
    float last_x = snake[0];
    float last_y = snake[1];
    /// iterate positions
    size_t i;
    for (i = 2; i < snake_size - 1; i += 2) { /// snake_size-1 because we need 2 items
        go_to_destination(snake[i], NAN, NAN, extrusion_Manhattan(snake, i, last_x), speed);
        last_x = snake[i];
        go_to_destination(NAN, snake[i + 1], NAN, extrusion_Manhattan(snake, i + 1, last_y), speed);
        last_y = snake[i + 1];
    }
    if (i == snake_size - 1) { /// process last X movement
        plan_destination(snake[i], NAN, NAN, extrusion_Manhattan(snake, i, last_x), speed);
    }
}

void FirstLayer::print_shape_1() {
    total_lines = 11 + ARRAY_SIZE(snake1);
    current_line = 0;

    /// print purge line
    go_to_destination(NAN, NAN, 4.f, NAN, 1000.f);
    go_to_destination(0.f, -2.f, 0.2f, NAN, 3000.f);
    go_to_destination(NAN, NAN, NAN, 6.f, 2000.f);
    go_to_destination(60.f, NAN, NAN, 9.f, 1000.f);
    go_to_destination(100.f, NAN, NAN, 12.5f, 1000.f);
    go_to_destination(NAN, NAN, 2.f, -6.f, 2100.f);

    /// go to starting point and de-retract
    go_to_destination(10.f, 150.f, 0.2f, NAN, 3000.f);
    go_to_destination(NAN, NAN, NAN, 6.f, 2000.f);
    go_to_destination(NAN, NAN, NAN, NAN, 1000.f);

    print_snake(snake1, ARRAY_SIZE(snake1), 1000.f);

    /// finish printing
    go_to_destination(NAN, NAN, 2.f, -6.f, 2100.f);
    go_to_destination(178.f, 0.f, 10.f, NAN, 3000.f);

    finish_printing();
}

void FirstLayer::print_shape_2() {
    enable_all_steppers();
    // M221 S100 ; reset flow
    planner.flow_percentage[0] = 100;
    planner.refresh_e_factor(0);
    /// fixed lines - constant to show 100% at the end + calibration pattern
    total_lines = 8 - 3 + ARRAY_SIZE(snake2);
    current_line = 0;

    /// print purge line
    go_to_destination(NAN, NAN, 4.f, NAN, 1000.f);
    go_to_destination(NAN, -2.f, 0.2f, NAN, 3000.f);
    go_to_destination(0.f, NAN, 0.2f, NAN, 3000.f);
    go_to_destination(NAN, NAN, NAN, 6.f, 2000.f);
    go_to_destination(60.f, NAN, NAN, 9.f, 1000.f);
    go_to_destination(100.f, NAN, NAN, 12.5f, 1000.f);
    go_to_destination(10.f, 30.f, NAN, extrusion(100.0f, -2.f, 10.f, 30.f), 1000.f);

    print_snake(snake2, ARRAY_SIZE(snake2), 1000.f);

    /// finish printing
    // go_to_destination(NAN, NAN, 2.f, -6.f, 2100.f);
    // go_to_destination(178.f, 180.f, 10.f, NAN, 3000.f);

    // TYPE:Custom
    //  Filament-specific end gcode
    // TODO setprecent? ////M73 P94 R0
    go_to_destination(NAN, NAN, NAN, -1.f, 2100.f); // G1 E-1 F2100 ; retract
    go_to_destination(NAN, NAN, 2.2f, NAN, 720.f); // G1 Z2.2 F720 ; Move print head up
    go_to_destination(178.f, 178.f, NAN, NAN, 4200.f); // G1 X178 Y178 F4200 ; park print head
    // TODO setprecent? ////M73 P96 R0
    go_to_destination(NAN, NAN, 30.2f, NAN, 720.f); // G1 Z30.2 F720 ; Move print head further up
    planner.synchronize(); // G4 ; wait .. finish moves == M400

    // no need lro reset linear advance, was not set // M900 K0 ; reset LA
    planner.finish_and_disable(); // M84 ; disable motors
    // TODO setprecent? // M73 P100 R0
    finish_printing();
}

/** \addtogroup G-Codes
 * @{
 */

/**
 * @brief gcode to draw a first layer on bed
 *
 * does not take any parameters
 * meant to be called from selftest
 */
void PrusaGcodeSuite::G26() {
    // is filament selected
    auto filament = config_store().get_filament_type(active_extruder);
    if (filament == FilamentType::none) {
        return;
    }

    FirstLayer fl;

    auto filament_description = filament::get_description(filament);
    const int temp_nozzle = filament_description.nozzle_temperature;

    // nozzle temperature print
    thermalManager.setTargetHotend(temp_nozzle, 0);
    marlin_server::set_temp_to_display(temp_nozzle, 0);
    thermalManager.wait_for_hotend(0, false);

    // fl.print_shape_1();
    fl.print_shape_2();

    thermalManager.setTargetHotend(0, 0);
    marlin_server::set_temp_to_display(0, 0);

    thermalManager.setTargetBed(0);
}

/** @}*/
