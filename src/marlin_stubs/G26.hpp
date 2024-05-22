/**
 * @file G26.hpp
 * @brief first layer calibration, must be run within selftest only
 */

#pragma once

#include "../../lib/Marlin/Marlin/src/module/planner.h"

/**
 * @brief ensures proper progress state in marlin_server
 */
class FirstLayerProgressLock {
    static uint32_t isPrinting_;

public:
    FirstLayerProgressLock() {
        ++isPrinting_;
    }

    ~FirstLayerProgressLock() {
        --isPrinting_;
    }

    static bool isPrinting() {
        return isPrinting_;
    }
};

class FirstLayer : public FirstLayerProgressLock {
private:
    static uint32_t finished_n_times;
    static uint32_t started_n_times;

    uint16_t total_lines = 1;
    uint16_t current_line = 0;
    uint8_t last_progress = 0;

    void finish_printing();

public:
    FirstLayer() {
        ++started_n_times;
    }

    ~FirstLayer() {
        ++finished_n_times;
        disable_all_steppers();
    }

    static uint32_t HowManyTimesFinished() {
        return finished_n_times;
    }

    static uint32_t HowManyTimesStarted() {
        return started_n_times;
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
