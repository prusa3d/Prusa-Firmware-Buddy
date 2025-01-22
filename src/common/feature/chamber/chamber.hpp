#pragma once

#include <optional>

#include <option/xl_enclosure_support.h>
#include <option/has_xbuddy_extension.h>
#include <common/temperature.hpp>
#include <freertos/mutex.hpp>

// TODO: Migrate XL Enclosure to use this API (& unify)
// TODO: Add support for controlling MK4 enclosure through GPIO expander

namespace buddy {

/// Everything here should be thread-safe
class Chamber {

public: // Common/utilities
    Chamber() {
        reset();
    }

    struct Capabilities {
        bool temperature_reporting = false;

        bool heating = false;
        bool cooling = false;

        /// Always show temperature control, even if temperature_control() == false
        /// In that situation, the temperature control widgets will be visible, but disabled
        bool always_show_temperature_control = false;

        std::optional<Temperature> max_temp = std::nullopt;

        inline bool temperature_control() const {
            return heating || cooling;
        }
    };

    /// \returns What capabilities the chamber has
    Capabilities capabilities() const;

    enum class Backend {
        none,
#if XL_ENCLOSURE_SUPPORT()
        xl_enclosure,
#endif
#if HAS_XBUDDY_EXTENSION()
        xbuddy_extension,
#endif
    };

    /// \returns the current backend that the chamber is using
    Backend backend() const;

    /// Does the chamber control logic
    /// !!! Only to be called from the marlin thread
    void step();

    /// Set the chamber to initial setup.
    ///
    /// Currently, resets the target temperature to no cooling.
    void reset();

public: // Temperature control
    std::optional<Temperature> current_temperature() const;

    std::optional<Temperature> thermistor_temperature() const;

    std::optional<Temperature> target_temperature() const;

    /// Sets the \param target temperature. Can be nullopt if we are not interested in controlling the temperature at all.
    void set_target_temperature(std::optional<Temperature> target);

private:
    mutable freertos::Mutex mutex_;

    std::optional<Temperature> thermistor_temperature_;
    std::optional<Temperature> target_temperature_;

    Capabilities capabilities_nolock(Backend backend) const;
};

Chamber &chamber();

} // namespace buddy
