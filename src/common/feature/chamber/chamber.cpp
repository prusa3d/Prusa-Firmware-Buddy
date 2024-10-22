#include "chamber.hpp"

#include <marlin_server_shared.h>

#if XL_ENCLOSURE_SUPPORT()
    #include <hw/xl/xl_enclosure.hpp>
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

#endif
}

Chamber::Capabilities Chamber::capabilities() const {
    std::lock_guard _lg(mutex_);

#if XL_ENCLOSURE_SUPPORT()
    return Capabilities {
        .temperature_reporting = xl_enclosure.isEnabled(),
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
