#include "chamber.hpp"

#include <cmath>

#include <marlin_server_shared.h>
#include <option/has_xbuddy_extension.h>
#include <buddy/unreachable.hpp>

#if XL_ENCLOSURE_SUPPORT()
    #include <hw/xl/xl_enclosure.hpp>
#endif

#if PRINTER_IS_PRUSA_COREONE()
    #include <marlin_server.hpp>
    #include <config_store/store_instance.hpp>
#endif

#if HAS_XBUDDY_EXTENSION()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
#endif

#if PRINTER_IS_PRUSA_COREONE()
    #define HAS_CHAMBER_TEMPERATURE_THERMISTOR_POSITION_OFFSET() 1
#elif PRINTER_IS_PRUSA_XL()
    #define HAS_CHAMBER_TEMPERATURE_THERMISTOR_POSITION_OFFSET() 0
#else
    #error
#endif

#if HAS_CHAMBER_TEMPERATURE_THERMISTOR_POSITION_OFFSET()
    #include <Configuration.h>
#endif

#if PRINTER_IS_PRUSA_COREONE()
namespace {
constexpr buddy::Temperature chamber_maxtemp = 60;
constexpr buddy::Temperature chamber_maxtemp_safety_margin = 5;
} // namespace
#endif

namespace buddy {

Chamber &chamber() {
    static Chamber instance;
    return instance;
}

void Chamber::step() {
    assert(osThreadGetId() == marlin_server::server_task);

    std::lock_guard _lg(mutex_);

#if XL_ENCLOSURE_SUPPORT()
    thermistor_temperature_ = xl_enclosure.getEnclosureTemperature();

#elif HAS_XBUDDY_EXTENSION()
    // Dummy, untested implementation.
    thermistor_temperature_ = xbuddy_extension().chamber_temperature();
#endif

    METRIC_DEF(metric_chamber_temp, "chamber_temp", METRIC_VALUE_FLOAT, 1000, METRIC_DISABLED);
    if (thermistor_temperature_.has_value()) {
        metric_record_float(&metric_chamber_temp, thermistor_temperature_.value());
    } else {
        metric_record_float(&metric_chamber_temp, NAN);
    }
}

Chamber::Capabilities Chamber::capabilities_nolock() const {
    switch (backend()) {

#if XL_ENCLOSURE_SUPPORT()
    case Backend::xl_enclosure:
        return Capabilities {
            .temperature_reporting = true,
        };
#endif

#if HAS_XBUDDY_EXTENSION()
    case Backend::xbuddy_extension:
        return Capabilities {
            .temperature_reporting = true,

            // The chamber can effectively control temperature only if the fans are in auto mode
                .cooling = (xbuddy_extension().fan_target_pwm(XBuddyExtension::Fan::cooling_fan_1) == pwm_auto),

            // But always show temperature control menu items, even if disabled
                .always_show_temperature_control = true,

    #if PRINTER_IS_PRUSA_COREONE()
            .max_temp = { chamber_maxtemp - chamber_maxtemp_safety_margin },
    #endif
        };
#endif

    case Backend::none:
        return Capabilities {};
    }
    BUDDY_UNREACHABLE();
}

Chamber::Capabilities Chamber::capabilities() const {
    std::lock_guard _lg(mutex_);

    return capabilities_nolock();
}

Chamber::Backend Chamber::backend() const {
#if XL_ENCLOSURE_SUPPORT()
    if (xl_enclosure.isEnabled()) {
        return Backend::xl_enclosure;
    }
#endif

#if HAS_XBUDDY_EXTENSION()
    if (xbuddy_extension().status() != XBuddyExtension::Status::disabled) {
        return Backend::xbuddy_extension;
    }
#endif

    return Backend::none;
}

std::optional<Temperature> Chamber::current_temperature() const {
    const auto chamber_tempearture = thermistor_temperature();
#if HAS_CHAMBER_TEMPERATURE_THERMISTOR_POSITION_OFFSET()
    #if PRINTER_IS_PRUSA_COREONE()
    const auto bed_temperature = thermalManager.degBed();
    static constexpr Temperature min_temp = 20.f;
    if (chamber_tempearture.has_value() && bed_temperature > *chamber_tempearture && *chamber_tempearture > min_temp) {
        static constexpr Temperature bed_max = BED_MAXTEMP - BED_MAXTEMP_SAFETY_MARGIN;
        static constexpr Temperature chamber_max = chamber_maxtemp;
        static constexpr Temperature offset = 6.f / ((bed_max - min_temp) * std::sqrt(chamber_max - min_temp));
        return chamber_tempearture.value() + offset * (bed_temperature - chamber_tempearture.value()) * std::sqrt(chamber_tempearture.value() - min_temp);
    }
    #else
        #error
    #endif
#endif
    return chamber_tempearture;
}

std::optional<Temperature> Chamber::thermistor_temperature() const {
    std::lock_guard _lg(mutex_);
    return thermistor_temperature_;
}

std::optional<Temperature> Chamber::target_temperature() const {
    std::lock_guard _lg(mutex_);
    return target_temperature_;
}

std::optional<Temperature> Chamber::set_target_temperature(std::optional<Temperature> target) {
    std::lock_guard _lg(mutex_);
    target_temperature_ = target;

    const auto max_temp = capabilities_nolock().max_temp;
    if (max_temp.has_value() && target_temperature_.has_value()) {
        target_temperature_ = std::min(*target_temperature_, *max_temp);
    }

    METRIC_DEF(metric_chamber_ttemp, "chamber_ttemp", METRIC_VALUE_FLOAT, 1000, METRIC_DISABLED);
    metric_record_float(&metric_chamber_ttemp, target_temperature_.value_or(NAN));

    return target_temperature_;
}

void Chamber::reset() {
    std::lock_guard _lg(mutex_);
    target_temperature_ = std::nullopt;

#if HAS_XBUDDY_EXTENSION()
    xbuddy_extension().set_fan_target_pwm(XBuddyExtension::Fan::cooling_fan_1, pwm_auto);
#endif
}

#if PRINTER_IS_PRUSA_COREONE()
void Chamber::check_vent_state() {
    const auto fil_target = config_store().get_filament_type(0).parameters().chamber_target_temperature;
    constexpr uint8_t temp_limit = 45; // Limit for closed grills is chamber max temperature of PETG

    // Don't show any vent dialog if filament doesn't support chamber temperature control
    if (fil_target.has_value()) {
        if (fil_target.value() > temp_limit && vent_state_ != Chamber::VentState::closed) {
            marlin_server::set_warning(WarningType::CloseChamberVents);
            vent_state_ = Chamber::VentState::closed;
        } else if (fil_target.value() <= temp_limit && vent_state_ != Chamber::VentState::open) {
            marlin_server::set_warning(WarningType::OpenChamberVents);
            vent_state_ = Chamber::VentState::open;
        }
    }
}
#endif

} // namespace buddy
