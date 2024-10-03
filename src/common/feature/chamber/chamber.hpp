#pragma once

#include <optional>

#include <common/temperature.hpp>

// TODO: Migrate XL Enclosure to use this API (& unify)
// TODO: Add support for controlling MK4 enclosure through GPIO expander

namespace buddy {

/// Everything here should be thread-safe
class Chamber {

public: // Common/utilities
    /// Does the chamber control logic
    /// !!! Only to be called from the marlin thread
    void step();

public: // Temperature control
    struct TemperatureControl {
        bool supports_heating = false;
        bool supports_cooling = false;

        bool operator==(const TemperatureControl &) const = default;
    };

    /// \returns What temperature control is the chamber capable of, if any
    TemperatureControl temperature_control() const;

    std::optional<Temperature> current_temperature() const;

    std::optional<Temperature> target_temperature() const;

    /// Sets the \param target temperature. Can be nullopt if we are not interested in controlling the temperature at all.
    void set_target_temperature(std::optional<Temperature> target);

private:
    mutable freertos::Mutex mutex_;

    std::optional<Temperature> current_temperature_;
    std::optional<Temperature> target_temperature_;
};

Chamber &chamber();

} // namespace buddy
