#pragma once

#include <optional>

#include <option/xl_enclosure_support.h>
#include <option/has_chamber_vents.h>
#include <option/has_xbuddy_extension.h>
#include <temperature.hpp>
#include <freertos/mutex.hpp>
#include <printers.h>
#include <utils/compact_optional.hpp>

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

        /// Maximum temperature the chamber is allowed to reach
        std::optional<Temperature> max_temp = std::nullopt;

        inline bool temperature_control() const {
            return heating || cooling;
        }
    };

    /// \returns What capabilities the chamber has
    Capabilities capabilities() const;

    enum class Backend : uint8_t {
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
    /// \returns the target temperature the chamber was actually set to - might differe because of capabilities().max_temp
    /// !!! MARLIN THREAD ONLY - we don't want to change it under g-code and thermal model hands
    std::optional<Temperature> set_target_temperature(std::optional<Temperature> target);

#if HAS_CHAMBER_VENTS()
    /// Check the state of chamber grills (vents). Can be open/closed based on chamber target temperature
    /// @param fil_target The target chamber temperature to base the vent decision on
    /// !HAS TO BE CALLED FROM DEFAULT THREAD ONLY!
    void manage_ventilation_state(std::optional<Temperature> fil_target);

    /// Close the vents at the end of a print, if they are known to be open
    /// and vent control is set to automatic.
    /// !HAS TO BE CALLED FROM DEFAULT THREAD ONLY!
    void close_vents_after_print();

    enum class VentState : uint8_t {
        open,
        closed,
    };

    inline void set_vent_state(VentState new_state) { vent_state_ = new_state; }
#endif

private:
    mutable freertos::Mutex mutex_;

    std::optional<Temperature> thermistor_temperature_;
    std::optional<Temperature> target_temperature_;

#if HAS_CHAMBER_VENTS()
    /// Nullopt = unknown state
    CompactOptional<VentState, static_cast<VentState>(0xff)> vent_state_ = std::nullopt;
#endif

    Capabilities capabilities_nolock() const;
};

Chamber &chamber();

} // namespace buddy
