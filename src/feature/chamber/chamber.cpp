#include "chamber.hpp"

#include <bsod/bsod.h>
#include <cmath>
#include <config_store/store_instance.hpp>
#include <marlin_server.hpp>
#include <marlin_server_shared.h>
#include <option/has_chamber_vents.h>
#include <option/has_xbuddy_extension.h>
#include <feature/safety_timer/safety_timer.hpp>
#include "chamber_enums.hpp"

#if HAS_CHAMBER_VENTS()
    #include <marlin_stubs/feature/automatic_chamber_vents/automatic_chamber_vents.hpp>
#endif

#if XL_ENCLOSURE_SUPPORT()
    #include <hw/xl/xl_enclosure.hpp>
#endif

#if HAS_XBUDDY_EXTENSION()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
#endif

#if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
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
#elif PRINTER_IS_PRUSA_COREONEL()
namespace {
constexpr buddy::Temperature chamber_maxtemp = 65;
constexpr buddy::Temperature chamber_maxtemp_safety_margin = 5;
} // namespace
#elif PRINTER_IS_PRUSA_XL()
// Not used for chamber on XL
#else
    #error
#endif

namespace buddy {

Chamber &chamber() {
    static Chamber instance;
    return instance;
}

void Chamber::step() {
    debug_assert(osThreadGetId() == marlin_server::server_task);

    std::lock_guard _lg(mutex_);

#if XL_ENCLOSURE_SUPPORT()
    thermistor_temperature_ = xl_enclosure.getEnclosureTemperature();

#elif HAS_XBUDDY_EXTENSION()
    // Dummy, untested implementation.
    thermistor_temperature_ = xbuddy_extension().chamber_temperature();
#endif

    METRIC_DEF(metric_chamber_temp, "chamber_temp", METRIC_VALUE_FLOAT, 1000, METRIC_ENABLED);
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
            .cooling = xbuddy_extension().can_auto_cool(),
            // Always show temperature control menu items, even if auto cooling is disabled
                .always_show_temperature_control = true,

    #if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
            .max_temp = { chamber_maxtemp - chamber_maxtemp_safety_margin },
    #endif
        };
#endif

    case Backend::none:
        return Capabilities {};
    }
    bsod_unreachable();
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
    #if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
    const auto bed_temperature = thermalManager.degBed();
    static constexpr Temperature min_temp = 20.f;
    if (chamber_tempearture.has_value() && bed_temperature > *chamber_tempearture && *chamber_tempearture > min_temp) {
        static constexpr Temperature bed_max = BED_MAXTEMP - BED_MAXTEMP_SAFETY_MARGIN;
        static constexpr Temperature chamber_max = chamber_maxtemp;
        #if PRINTER_IS_PRUSA_COREONEL()
        static constexpr Temperature offset = 8.f / ((bed_max - min_temp) * std::sqrt(chamber_max - min_temp));
        #else
        static constexpr Temperature offset = 6.f / ((bed_max - min_temp) * std::sqrt(chamber_max - min_temp));
        #endif
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
    debug_assert(marlin_server::is_marlin_server_thread());

    // Wake up heaters if they are timed out
    buddy::safety_timer().reset_restore_nonblocking();

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
    xbuddy_extension().set_fan_target_pwm(XBuddyExtension::Fan::filtration_fan, pwm_auto);
#endif
}

#if HAS_CHAMBER_VENTS()
void Chamber::manage_ventilation_state(std::optional<Temperature> fil_target) {
    if (!fil_target.has_value()) {
        // We don't know whether we should close or open, leave as is
        return;
    }

    constexpr uint8_t temp_limit = 45; // Limit for closed grills is chamber max temperature of PETG
    const auto target_state = (fil_target.value() > temp_limit) ? VentState::closed : VentState::open;

    if (vent_state_ == target_state) {
        // Vents at the correct position
        return;
    }

    switch (config_store().get_vent_control()) {

    case VentControl::off:
        return;

    case VentControl::automatic:
        automatic_chamber_vents::execute_control(target_state);
        break;

    case VentControl::manual:
        marlin_server::set_warning(target_state == VentState::open ? WarningType::OpenChamberVents : WarningType::CloseChamberVents);
        vent_state_ = target_state;
        break;
    }
}

void Chamber::close_vents_after_print() {
    if (vent_state_ != VentState::open) {
        return;
    }

    if (config_store().get_vent_control() != VentControl::automatic) {
        return;
    }

    automatic_chamber_vents::execute_control(VentState::closed);
}
#endif

} // namespace buddy
