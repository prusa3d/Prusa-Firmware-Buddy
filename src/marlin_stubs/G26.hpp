// G26.hpp

#pragma once

#include "../../lib/Marlin/Marlin/src/module/planner.h"

class FirstLayer : public FSM_Holder {
private:
    static bool isPrinting_; /// ensures proper progress state in marlin_server

    uint16_t total_lines = 1;
    uint16_t current_line = 0;
    uint8_t last_progress = 0;

    void finish_printing();

public:
    FirstLayer()
        : FSM_Holder(ClientFSM::FirstLayer, 0) { isPrinting_ = true; }

    ~FirstLayer() { isPrinting_ = false; }

    static bool isPrinting() {
        return isPrinting_;
    }

    void wait_for_move() {
        planner.synchronize();
    }

    /// Moves head and extrudes
    /// Use NAN for axis you don't want to move with
    /// \param e is relative extrusion
    /// \param f is defined in millimeters per minute (like in G code)
    void go_to_destination(const float x, const float y, const float z, const float e, const float f);

    /// increases progress by 1 line and sends it to Marlin
    void inc_progress();

    /// Puts the destination into the Marlin planner and waits for the end of the move
    void go_to_destination_and_wait(const float x, const float y, const float z, const float e, const float f);

    /// Moves and extrudes
    /// Keep Z and feedrate from the last time
    void go_to_destination(const float x, const float y, const float e) {
        go_to_destination(x, y, NAN, e, NAN);
    }

    /// @returns length of filament to extrude
    constexpr float extrusion(const float x1, const float y1, const float x2, const float y2, const float layerHeight = 0.2f, const float threadWidth = 0.5f) const;

    /// @returns length of filament to extrude on the Manhattan path
    constexpr float extrusion_Manhattan(const float *path, const uint32_t position, const float last) const;

    void print_snake(const float *snake, const size_t snake_size, const float speed);
    void print_shape_1();
    void print_shape_2();
};

/// Path of Manhattan snake
/// Alternate X and Y coordinates.
/// It cannot print diagonals etc.
/// First X and Y are a starting point.
static const constexpr float snake1[] = {
    /// use 0.5 extrusion width
    10,  /// start X
    150, /// start Y
    170, /// X
    130, /// Y
    10,  /// X
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

/// Path of Manhattan snake
/// Alternate X and Y coordinates.
/// It cannot print diagonals etc.
/// First X and Y are a starting point.
static const constexpr float snake2[] = {
    /// use 0.5 extrusion width
    10,        /// start X
    180 - 150, /// start Y
    170,       /// X
    180 - 130, /// Y
    10,        /// X
    180 - 110, /// ...
    170,
    180 - 90,
    10,
    180 - 70,
    170,
    180 - 50,
    ///
    /// frame around
    9.5,
    180 - 17,
    30.5,
    180 - 30.5,
    10,
    180 - 30,
    ///
    /// infill
    30,
    180 - 29.5,
    10,
    180 - 29,
    30,
    180 - 28.5,
    10,
    180 - 28,
    30,
    180 - 27.5,
    10,
    180 - 27,
    30,
    180 - 26.5,
    10,
    180 - 26,
    30,
    180 - 25.5,
    10,
    180 - 25,
    30,
    180 - 24.5,
    10,
    180 - 24,
    30,
    180 - 23.5,
    10,
    180 - 23,
    30,
    180 - 22.5,
    10,
    180 - 22,
    30,
    180 - 21.5,
    10,
    180 - 21,
    30,
    180 - 20.5,
    10,
    180 - 20,
    30,
    180 - 19.5,
    10,
    180 - 19,
    30,
    180 - 18.5,
    10,
    180 - 18,
    30,
    180 - 17.5,
    10,
};
