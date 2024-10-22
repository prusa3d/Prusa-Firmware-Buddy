#include "chamber.hpp"

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
}

Chamber::Capabilities Chamber::capabilities() const {
    std::lock_guard _lg(mutex_);

#if XL_ENCLOSURE_SUPPORT()
    return Capabilities {
        .temperature_reporting = xl_enclosure.isEnabled(),
    };
#endif

#if HAS_XBUDDY_EXTENSION()
    return Capabilities {
        .temperature_reporting = true,
        .cooling = xbuddy_extension().has_fan1_fan2_auto_control(),
    };
#endif

    return Capabilities {};
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
}

void Chamber::reset() {
    std::lock_guard _lg(mutex_);
    target_temperature_ = std::nullopt;
}

} // namespace buddy
