#include "menu_items_chamber_filtration.hpp"

#include <algorithm_extensions.hpp>

#include <feature/chamber/chamber.hpp>
#include <numeric_input_config_common.hpp>

using namespace buddy;

namespace {
constexpr NumericInputConfig power_numeric_config {
    .max_value = 100,
    .step = 5,
    .special_value = 0,
    .unit = Unit::percent,
};

constexpr NumericInputConfig duration_numeric_config {
    .min_value = 1,
    .max_value = 30,
    .unit = Unit::minute,
};
} // namespace

// MI_CHAMBER_FILTRATION_BACKEND
// ============================================
MI_CHAMBER_FILTRATION_BACKEND::MI_CHAMBER_FILTRATION_BACKEND()
    : MenuItemSelectMenu(_("Chamber Filtration")) {
    item_count_ = ChamberFiltration::get_available_backends(items_);

    const auto backend = config_store().chamber_filtration_backend.get();
    const auto ix = stdext::index_of(std::span(items_.data(), item_count_), backend);
    if (ix == item_count_) {
        items_[item_count_++] = backend;
    }
    set_current_item(ix);
}

void MI_CHAMBER_FILTRATION_BACKEND::build_item_text(int index, const std::span<char> &buffer) const {
    _(ChamberFiltration::backend_name(items_[index])).copyToRAM(buffer);
}

bool MI_CHAMBER_FILTRATION_BACKEND::on_item_selected(int, int new_index) {
    config_store().chamber_filtration_backend.set(items_[new_index]);
    return true;
}

// MI_CHAMBER_PRINT_FILTRATION_POWER
// ============================================
MI_CHAMBER_PRINT_FILTRATION_POWER::MI_CHAMBER_PRINT_FILTRATION_POWER()
    : WiSpin(config_store().chamber_mid_print_filtration_pwm.get().to_percent(), power_numeric_config, _("Mid-print Power")) {}

void MI_CHAMBER_PRINT_FILTRATION_POWER::OnClick() {
    config_store().chamber_mid_print_filtration_pwm.set(PWM255::from_percent(value()));
}

// MI_CHAMBER_POST_PRINT_FILTRATION_POWER
// ============================================
MI_CHAMBER_POST_PRINT_FILTRATION_POWER::MI_CHAMBER_POST_PRINT_FILTRATION_POWER()
    : WiSpin(config_store().chamber_post_print_filtration_pwm.get().to_percent(), power_numeric_config, _("Post-print Power")) {}

void MI_CHAMBER_POST_PRINT_FILTRATION_POWER::OnClick() {
    config_store().chamber_post_print_filtration_pwm.set(PWM255::from_percent(value()));
}

// MI_CHAMBER_POST_PRINT_FILTRATION_DURATION
// ============================================
MI_CHAMBER_POST_PRINT_FILTRATION_DURATION::MI_CHAMBER_POST_PRINT_FILTRATION_DURATION()
    : WiSpin(config_store().chamber_post_print_filtration_duration_min.get(), duration_numeric_config, _("Post-print Duration")) {}

void MI_CHAMBER_POST_PRINT_FILTRATION_DURATION::OnClick() {
    config_store().chamber_post_print_filtration_duration_min.set(value());
}
