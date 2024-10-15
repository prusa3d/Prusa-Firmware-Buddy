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

    /// Target-current temperature difference at which the fans go on full
    static constexpr int fans_max_temp_diff = 10;
    const auto target_fan_pwm = //
        (!current_temperature_.has_value() || !target_temperature_.has_value())

        // We don't know a temperature or don't have a target set -> do not cool
        ? 0

        // Linearly increase fans up to the fans_max_temp_diff temperature difference
        : std::clamp<int>(static_cast<int>(*current_temperature_ - *target_temperature_) * 255 / fans_max_temp_diff, 0, 255);

    xbuddy_extension().set_fan1_fan2_pwm(target_fan_pwm);

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
        .cooling = true,
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

} // namespace buddy
