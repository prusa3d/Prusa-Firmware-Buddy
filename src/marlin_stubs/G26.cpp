#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/feature/host_actions.h"
#include "../../lib/Marlin/Marlin/src/feature/safety_timer.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../lib/Marlin/Marlin/src/Marlin.h"
#include "marlin_server.hpp"
#include "client_fsm_types.h"
#include "PrusaGcodeSuite.hpp"
#include "G26.hpp"

void go_to_destination(const float x, const float y, const float z, const float e, const float f) {
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
        destination[3] = e;
    else
        destination[3] = current_position[3];

    if (isfinite(f))
        feedrate_mm_s = f;

    prepare_move_to_destination();
}

/// Keep Z and feedrate from last time
void go_to_destination(const float x, const float y, const float e) {
    if (isfinite(x))
        destination[0] = x;
    else
        destination[0] = current_position[0];

    if (isfinite(y))
        destination[1] = y;
    else
        destination[1] = current_position[1];

    destination[2] = current_position[2];

    if (isfinite(e))
        destination[3] = e;
    else
        destination[3] = current_position[3];

    prepare_move_to_destination();
}

float extrusion(const float *snake, const int position) {
    /// TODO implement
    return 0;
}

void print_snake() {
    /// move to start
    go_to_destination(snake1[0], snake1[1], 0); // Process X Y Z E F parameters
    /// iterate positions
    for (uint32_t i = 2; i < sizeof(snake1); i += 2) {
        go_to_destination(snake1[i], NAN, extrusion(snake1, i));
        go_to_destination(NAN, snake1[i + 1], extrusion(snake1, i + 1));
    }
}

void PrusaGcodeSuite::G26() {
    fsm_create(ClientFSM::FirstLayer);

    /// start movement

    // "G1 Z4 F1000",
    do_blocking_move_to_z(4, 1000);
    // "G1 X0 Y-2 Z0.2 F3000.0",
    // "G1 E6 F2000",
    // "G1 X60 E9 F1000.0",
    // "G1 X100 E12.5 F1000.0",
    // "G1 Z2 E-6 F2100.00000",

    // "G1 X10 Y150 Z0.2 F3000",
    // "G1 E6 F2000"

    // "G1 F1000",
    // //E = extrusion_length * layer_height * extrusion_width / (PI * pow(1.75, 2) / 4)

    /// snake extrusion (no Z movement)
    print_snake();

    /// finish printing

    // "G1 Z2 E-6 F2100",
    // "G1 X178 Y0 Z10 F3000",

    /// don't bother with G4 or heating turning off

    //
    // fsm_destroy(ClientFSM::FirstLayer);
}
