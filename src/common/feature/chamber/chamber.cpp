#include "chamber.hpp"

#include <marlin_server_shared.h>

#if PRINTER_IS_PRUSA_XL()
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

#if PRINTER_IS_PRUSA_XL()
    current_temperature_ = xl_enclosure.getEnclosureTemperature();
    if (current_temperature_ == Enclosure::INVALID_TEMPERATURE) {
        current_temperature_ = std::nullopt;
    }

#endif
}

Chamber::TemperatureControl Chamber::temperature_control() const {
    std::lock_guard _lg(mutex_);

    return TemperatureControl {};
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
