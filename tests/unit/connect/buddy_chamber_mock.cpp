#include <feature/chamber/chamber.hpp>

namespace buddy {

std::optional<Temperature> Chamber::set_target_temperature(std::optional<Temperature> target) {
    target_temperature_ = target;
    return target;
}

std::optional<Temperature> Chamber::target_temperature() const {
    return target_temperature_;
}

void Chamber::reset() {
    target_temperature_.reset();
}

Chamber &chamber() {
    static Chamber c;
    return c;
}

} // namespace buddy
