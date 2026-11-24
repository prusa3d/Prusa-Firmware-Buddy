/**
 * @file G26.hpp
 * @brief First layer calibration pattern (G26); used by the first layer selftest, also a public G-code
 */

#pragma once

#include "../../lib/Marlin/Marlin/src/module/planner.h"
#include <bsod/bsod.h>
#include <tool_index.hpp>

#include <optional>

class FirstLayer {
private:
    static FirstLayer *instance_;

    uint16_t total_lines = 1;
    uint16_t current_line = 0;

    void finish_printing();

public:
    FirstLayer() {
        debug_assert(!instance_);
        instance_ = this;
    }

    ~FirstLayer() {
        debug_assert(instance_ == this);
        instance_ = nullptr;
        disable_all_steppers();
    }

    static FirstLayer *instance() {
        return instance_;
    }

    /// Virtual tool whose filament parameters the pattern is printed with.
    ///
    /// The selected virtual tool, or on MMU printers slot 0 when no slot is selected: the MMU forgets the
    /// slot across a reset while the filament stays loaded. Slot 0 is what the selftest's filament check
    /// reads and what M702 falls back to.
    /// nullopt only on a toolchanger with no tool picked (MMU and toolchanger are mutually exclusive).
    [[nodiscard]] static std::optional<VirtualToolIndex> virtual_tool();

    uint8_t progress_percent() const;
    void run();
    void wait_for_move() {
        planner.synchronize();
    }

    /// Puts the destination into the Marlin planner without waiting for the move to end
    /// Use NAN for axis you don't want to move with
    /// \param e is relative extrusion
    /// \param f is defined in millimeters per minute (like in G code)
    void plan_destination(const float x, const float y, const float z, const float e, const float f);

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
