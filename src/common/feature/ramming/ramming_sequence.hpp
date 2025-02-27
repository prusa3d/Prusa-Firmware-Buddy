#pragma once

#include <span>
#include <array>
#include <inplace_function.hpp>
#include <cstdint>

namespace buddy {

class RammingSequence {

public:
    using EDistance = int16_t;

    struct Step {
        EDistance e; ///< relative movement of Extruder
        int16_t fr_mm_min; ///< feedrate of the move, in mm/min
    };

    using Steps = std::span<const Step>;

public:
    constexpr const Steps &steps() const {
        return steps_;
    }

    // !!! WARNING: Opposite to the original unload_length, retracted_distance is ALWAYS >= 0
    /// \returns the actual amount of the filament retracted by the sequence (how much deretraction will be needed)
    constexpr EDistance retracted_distance() const {
        return retracted_distance_;
    }

    using InterruptCallback = stdext::inplace_function<bool()>;

    /// Blockingly executes the ramming sequence. Stops if \param callback returns false.
    /// \returns true if the sequence fully finishes
    bool execute(const InterruptCallback &callback = [] { return true; }) const;

    /// Blockingly executes a simple "reverse" of the ramming sequence, getting the filament into the nozzle again.
    /// This equates extruding back \p for retracted_distance()
    /// \returns true if the sequence fully finishes
    bool undo() const;

protected:
    // Only intended to be used from RammingSequenceArray
    consteval RammingSequence() = default;

    consteval void init(const Steps &steps) {
        steps_ = steps;

        retracted_distance_ = 0;
        for (const Step &step : steps) {
            retracted_distance_ -= step.e;

            // If we ever get below zero, that means that we have extruded out of the nozzle.
            // The retracted distance should reset at this point
            if (retracted_distance_ < 0) {
                retracted_distance_ = 0;
            }
        }
    }

private:
    Steps steps_;
    EDistance retracted_distance_;
};

template <size_t step_count_>
class RammingSequenceArray final : public RammingSequence {

public:
    consteval RammingSequenceArray(const Step (&steps)[step_count_])
        : steps_array_(std::to_array(steps)) {
        init(steps_array_);
    }

private:
    std::array<Step, step_count_> steps_array_;
};

}; // namespace buddy
