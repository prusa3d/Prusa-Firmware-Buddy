/// @file
#pragma once

#include <tool_index.hpp>
#include <utils/uncopyable.hpp>
#include <utils/variant_utils.hpp>
#include <utils/compact_optional.hpp>
#include <module/temperature/hotend_regulator/hotend_regulator.hpp>
#include <module/temperature/temp_defines.hpp>
#include <pwm_utils.hpp>
#include <atomic>
#include <option/has_indx.h>

namespace buddy::filament_compatibility {
struct CompatibilityReport;
struct CompatibilityReportGenerateArgs;
} // namespace buddy::filament_compatibility

/// Class representing a hotend
/// This is an abstract class, hotend implementations differ
class Hotend : public Uncopyable {
    friend class Temperature;

public:
    /// in °C
    using Temperature = float;
    using OptionalTemperature = CompactOptional<Temperature, NAN>;

    /// in °C
    /// <= 0 = no target temperature/invalid value
    using TargetTemperature = int16_t;

    /// Maps a requested print-fan PWM to the PWM actually applied, given the hotend's live state
    /// (e.g. nozzle temperature). Default: identity. A hotend that must protect the print fan at
    /// high nozzle temperatures installs a clamping mapping alongside its config (see tools_xbuddy).
    using PrintFanPWMMapping = PWM255 (*)(const Hotend &hotend, PWM255 requested_pwm);

    /// Static, per-hotend configuration common to all Hotend implementations.
    /// The owning tool factory keeps the instance alive; Hotend stores a reference to it.
    struct Config {
        /// Minimum acceptable temperature for the hotend
        /// Exceeding this limit results in a RSOD
        /// Formerly done by the HEATER_0_MINTEMP macro
        TargetTemperature min_nozzle_temp;

        /// Maximum acceptable temperature for the hotend
        /// Exceeding this limit results in a RSOD
        /// Formerly done by the HEATER_0_MAXTEMP macro
        TargetTemperature max_nozzle_temp;

        /// Print-fan PWM mapping (e.g. high-temperature clamp). Default: identity, no limiting.
        PrintFanPWMMapping print_fan_pwm_mapping = +[](const Hotend &, PWM255 pwm) { return pwm; };
    };

public:
    /// @returns Hotend of the tool
    /// !!! To be accessed only from the marlin task
    static Hotend &for_tool(PhysicalToolIndex tool);

    static Hotend &for_tool(std::variant<PhysicalToolIndex, NoTool> tool);

    [[deprecated("Use the strong typed variant")]]
    static Hotend &for_tool(uint8_t tool);

public:
    using FilamentCompatibilityReport = buddy::filament_compatibility::CompatibilityReport;
    using FilamentCompatibilityReportGenerateArgs = buddy::filament_compatibility::CompatibilityReportGenerateArgs;

    /// !!! MUST BE THREAD-SAFE, CAN BE CALLED FROM ANY THREAD
    virtual void filament_compatibility_report(FilamentCompatibilityReport &report, const FilamentCompatibilityReportGenerateArgs &args) const = 0;

    /// Maximum nozzle temperature (from the hotend config).
    /// DummyHotend (NoTool) is constructed with a zero config, so this returns 0.
    TargetTemperature max_nozzle_temp() const { return base_config_.max_nozzle_temp; }

    /// The static per-hotend config (temperature limits, print-fan PWM mapping, ...).
    const Config &config() const { return base_config_; }

    /// Nozzle temperature above which a burn risk warning is shown when the door opens.
    /// A burn is a burn regardless of hotend model, so this is a fixed constant rather
    /// than per-hotend data — barely reachable by the standard hotend, relevant for HT.
    static constexpr TargetTemperature burn_warning_temp = 290;

    /// Current temperature of the nozzle
    OptionalTemperature nozzle_temp() const {
        return nozzle_temp_;
    }

    /// @returns true when this hotend is currently being thermally controlled.
    /// Hotends that aren't thermally managed are excluded from temp wait/reached checks.
    bool is_thermally_managed() const { return is_thermally_managed_; }

    /// @returns whether the nozzle temperature has stabilized on the target,
    /// or the hotend is not thermally managed (so it shouldn't block global waits).
    bool is_nozzle_temp_reached() const {
        return !is_thermally_managed() || nozzle_temp_reached_;
    }

    /// Target temperature of the nozzle
    TargetTemperature nozzle_target_temp() const {
        return nozzle_target_temp_;
    }

    virtual void set_nozzle_target_temp(TargetTemperature set) = 0;

    const HotendPIDConfig &nozzle_pid_config() const {
        return nozzle_pid_config_;
    }

    virtual void set_nozzle_pid_config(const HotendPIDConfig &set) {
        nozzle_pid_config_ = set;
    }

    /// Compatibility function for heater selftests
    PID_t nozzle_pid_config_compat() const;

    /// Compatibility function for heater selftests
    void set_nozzle_pid_config_compat(const PID_t &set);

#if ENABLED(MODEL_DETECT_STUCK_THERMISTOR)
    bool is_thermal_model_protection_ok() const {
        return thermal_model_protection_ok_;
    }
#endif

    PWM255 nozzle_heater_pwm() const {
        return PWM255(nozzle_heater_pwm_);
    }

#if HAS_TEMP_HEATBREAK
    Temperature heatbreak_temp() const {
        return heatbreak_temp_;
    }
#endif

#if HAS_TEMP_HEATBREAK_CONTROL
    TargetTemperature heatbreak_target_temp() const {
        return heatbreak_target_temp_;
    }

    virtual void set_heatbreak_target_temp(TargetTemperature set) = 0;

    PWM255 heatbreak_fan_pwm() const {
        return heatbreak_fan_pwm_;
    }
#endif

protected:
    explicit Hotend(const Config &config)
        : base_config_(config) {}

protected:
    /// This function is called from the DefaultTask at regular intervals (from temperature.manage_heater())
    virtual void manage() = 0;

    /// Raw values are accumulated by the Temperature::isr to temp_hotend.acc
    /// Once there's OVERSAMPLENR values accumulated, this function is called
    /// It is supposed to pass the accumulated values to the defaultTask
    /// And reset the accumulators
    virtual void isr_on_readings_ready() {};

    /// Called from TemperatureISR to control bitbanged PWMs, if the hotend needs it.
    /// @param phase deterines the current value of the soft PWM counter. Pins should output high if phase <= pin_pwm_target
    virtual void isr_soft_pwm(PWM255 phase) { (void)phase; }

protected:
    HotendPIDConfig nozzle_pid_config_;
    OptionalTemperature nozzle_temp_ = std::nullopt; // temp uninitialized

#if HAS_TEMP_HEATBREAK
    Temperature heatbreak_temp_ = TempInfo::celsius_uninitialized;
#endif

    TargetTemperature nozzle_target_temp_ = 0;

    /// Static per-hotend configuration; owned by the tool factory, referenced here.
    const Config &base_config_;

#if HAS_TEMP_HEATBREAK_CONTROL
    TargetTemperature heatbreak_target_temp_ = 0;
#endif

    /// Output power of the nozzle heater
    /// For local hotends, this is set in Hotend::manage and potentially used for soft pwm control
    /// For remote hotends, this is retrieved from the remote board and only used for display/reporting purposes
    /// Possibly accessed from isr_soft_pwm, thus needs to be atomic
    std::atomic<uint8_t> nozzle_heater_pwm_ = 0;

#if HAS_TEMP_HEATBREAK
    PWM255 heatbreak_fan_pwm_;
#endif

    bool nozzle_temp_reached_ : 1 = false;

#if HAS_INDX()
    bool is_thermally_managed_ : 1 = true;
#else
    // The printer doesn't have a disablable hotend,
    // optimize out the ifs and prevent disabling safety checks
    static constexpr bool is_thermally_managed_ = true;
#endif

#if ENABLED(MODEL_DETECT_STUCK_THERMISTOR)
    bool thermal_model_protection_ok_ : 1 = false;
#endif
};
