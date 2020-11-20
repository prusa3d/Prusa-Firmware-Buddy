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

    ~FirstLayer() {
        isPrinting_ = false;
        disable_all_steppers();
    }

    static bool isPrinting() {
        return isPrinting_;
    }

    void wait_for_move() {
        planner.synchronize();
    }

    /// Puts the destination into the Marlin planner without waiting for the move to end
    /// Use NAN for axis you don't want to move with
    /// \param e is relative extrusion
    /// \param f is defined in millimeters per minute (like in G code)
    void plan_destination(const float x, const float y, const float z, const float e, const float f);

    /// increases progress by 1 line and sends it to Marlin
    void inc_progress();

    /// Puts the destination into the Marlin planner and waits for the end of the move
    void go_to_destination(const float x, const float y, const float z, const float e, const float f);

    /// Puts the destination into the Marlin planner without waiting for the move to end
    /// \param e is relative extrusion
    /// Keep Z and feedrate from the last time
    void plan_destination(const float x, const float y, const float e) {
        plan_destination(x, y, NAN, e, NAN);
    }

    /// @returns length of filament to extrude
    constexpr float extrusion(const float x1, const float y1, const float x2, const float y2, const float layerHeight = 0.2f, const float threadWidth = 0.5f) const;

    /// @returns length of filament to extrude on the Manhattan path
    constexpr float extrusion_Manhattan(const float *path, const uint32_t position, const float last) const;

    void print_snake(const float *snake, const size_t snake_size, const float speed);
    void print_shape_1();
    void print_shape_2();
};
