#include "chamber.hpp"

#include <cmath>

#include <marlin_server_shared.h>
#include <option/has_xbuddy_extension.h>

#if XL_ENCLOSURE_SUPPORT()
    #include <hw/xl/xl_enclosure.hpp>
#endif

#if HAS_XBUDDY_EXTENSION()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
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
    current_temperature_ = xl_enclosure.getEnclosureTemperature();

#elif HAS_XBUDDY_EXTENSION()
    // Dummy, untested implementation.
    current_temperature_ = xbuddy_extension().chamber_temperature();
#endif

    METRIC_DEF(metric_chamber_temp, "chamber_temp", METRIC_VALUE_FLOAT, 1000, METRIC_DISABLED);
    if (current_temperature_.has_value()) {
        metric_record_float(&metric_chamber_temp, current_temperature_.value());
    } else {
        metric_record_float(&metric_chamber_temp, NAN);
    }
}

Chamber::Capabilities Chamber::capabilities() const {
    std::lock_guard _lg(mutex_);

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
            .cooling = xbuddy_extension().has_fan1_fan2_auto_control(),

            // But always show temperature control menu items, even if disabled
            .always_show_temperature_control = true,
        };
#endif

    case Backend::none:
        break;
    }

    return Capabilities {};
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
    std::lock_guard _lg(mutex_);
    return current_temperature_;
}

std::optional<Temperature> Chamber::target_temperature() const {
    std::lock_guard _lg(mutex_);
    return target_temperature_;
}

void Chamber::set_target_temperature(std::optional<Temperature> target) {
    std::lock_guard _lg(mutex_);
    target_temperature_ = target;
    METRIC_DEF(metric_chamber_ttemp, "chamber_ttemp", METRIC_VALUE_FLOAT, 1000, METRIC_DISABLED);
    if (target_temperature_.has_value()) {
        metric_record_float(&metric_chamber_ttemp, target_temperature_.value());
    } else {
        metric_record_float(&metric_chamber_ttemp, NAN);
    }
}

void Chamber::reset() {
    std::lock_guard _lg(mutex_);
    target_temperature_ = std::nullopt;

#if HAS_XBUDDY_EXTENSION()
    xbuddy_extension().set_fan1_fan2_auto_control();
#endif
}

} // namespace buddy
