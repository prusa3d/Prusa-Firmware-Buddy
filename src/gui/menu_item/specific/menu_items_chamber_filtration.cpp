#include "menu_items_chamber_filtration.hpp"

#include <algorithm_extensions.hpp>

#include <feature/chamber/chamber.hpp>
#include <numeric_input_config_common.hpp>

#include <screen_change_filter.hpp>
#include <ScreenHandler.hpp>
#include <option/xl_enclosure_support.h>

using namespace buddy;

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

string_view_utf8 MI_CHAMBER_FILTRATION_BACKEND::build_item_text(int index, [[maybe_unused]] ItemTextParams &params) const {
    return _(ChamberFiltration::backend_name(items_[index]));
}

bool MI_CHAMBER_FILTRATION_BACKEND::on_item_selected(const OnItemSelectedArgs &args) {
    chamber_filtration().set_backend(items_[args.new_index]);
    return true;
}

// MI_CHAMBER_PRINT_FILTRATION
// ============================================
MI_CHAMBER_PRINT_FILTRATION::MI_CHAMBER_PRINT_FILTRATION()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().chamber_print_filtration_enable.get(), _("Print Filtration")) {}

void MI_CHAMBER_PRINT_FILTRATION::OnChange(size_t) {
    config_store().chamber_print_filtration_enable.set(value());
}

// MI_CHAMBER_PRINT_FILTRATION_POWER
// ============================================
static constexpr NumericInputConfig print_power_numeric_config {
    .min_value = 5,
    .max_value = 100,
    .step = 5,
    .unit = Unit::percent,
};

MI_CHAMBER_PRINT_FILTRATION_POWER::MI_CHAMBER_PRINT_FILTRATION_POWER()
    : WiSpin(config_store().chamber_mid_print_filtration_pwm.get().to_percent(), print_power_numeric_config, _("Minimum Power")) {}

void MI_CHAMBER_PRINT_FILTRATION_POWER::OnClick() {
    config_store().chamber_mid_print_filtration_pwm.set(PWM255::from_percent(static_cast<int8_t>(value())));
}

void MI_CHAMBER_PRINT_FILTRATION_POWER::Loop() {
    set_enabled(config_store().chamber_print_filtration_enable.get());
}

// MI_CHAMBER_POST_PRINT_FILTRATION
// ============================================
MI_CHAMBER_POST_PRINT_FILTRATION::MI_CHAMBER_POST_PRINT_FILTRATION()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().chamber_post_print_filtration_enable.get(), _("Post-print Filtration")) {}

void MI_CHAMBER_POST_PRINT_FILTRATION::OnChange(size_t) {
    config_store().chamber_post_print_filtration_enable.set(value());
}

// MI_CHAMBER_POST_PRINT_FILTRATION_POWER
// ============================================
static constexpr NumericInputConfig post_print_power_numeric_config {
    .min_value = 5,
    .max_value = 100,
    .step = 5,
    .unit = Unit::percent,
};

MI_CHAMBER_POST_PRINT_FILTRATION_POWER::MI_CHAMBER_POST_PRINT_FILTRATION_POWER()
    : WiSpin(config_store().chamber_post_print_filtration_pwm.get().to_percent(), post_print_power_numeric_config, _("Post-print Power")) {}

void MI_CHAMBER_POST_PRINT_FILTRATION_POWER::OnClick() {
    config_store().chamber_post_print_filtration_pwm.set(PWM255::from_percent(static_cast<int8_t>(value())));
}

void MI_CHAMBER_POST_PRINT_FILTRATION_POWER::Loop() {
    set_enabled(config_store().chamber_post_print_filtration_enable.get());
}

// MI_CHAMBER_POST_PRINT_FILTRATION_DURATION
// ============================================
static constexpr NumericInputConfig duration_numeric_config {
    .min_value = 1,
    .max_value = ChamberFiltration::max_post_print_filtration_time_min,
    .unit = Unit::minute,
};

MI_CHAMBER_POST_PRINT_FILTRATION_DURATION::MI_CHAMBER_POST_PRINT_FILTRATION_DURATION()
    : WiSpin(config_store().chamber_post_print_filtration_duration_min.get(), duration_numeric_config, _("Post-print Duration")) {}

void MI_CHAMBER_POST_PRINT_FILTRATION_DURATION::OnClick() {
    config_store().chamber_post_print_filtration_duration_min.set(static_cast<uint8_t>(value()));
}

void MI_CHAMBER_POST_PRINT_FILTRATION_DURATION::Loop() {
    set_enabled(config_store().chamber_post_print_filtration_enable.get());
}

// MI_CHAMBER_ALWAYS_FILTER
// ============================================
MI_CHAMBER_ALWAYS_FILTER::MI_CHAMBER_ALWAYS_FILTER()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().chamber_filtration_always_on.get(), _("Filter All Materials")) {}

void MI_CHAMBER_ALWAYS_FILTER::OnChange(size_t) {
    config_store().chamber_filtration_always_on.set(value());
}

// MI_CHAMBER_FILTER_TIME_USED
// ============================================
static constexpr NumericInputConfig filter_usage_numeric_config {
    .min_value = 0,
    .max_value = 9999,
    .unit = Unit::hour,
};

MI_CHAMBER_FILTER_TIME_USED::MI_CHAMBER_FILTER_TIME_USED()
    : WiSpin(0, filter_usage_numeric_config, _("Filter Usage")) {}

void MI_CHAMBER_FILTER_TIME_USED::OnClick() {
    // Allow the user to change this value. It is informative for the user comfort and there is no reason to disallow adjusting if it the user decides so
    // At the very least, this will be handy for testing purposes
    config_store().chamber_filter_time_used_s.set(static_cast<uint32_t>(value() * 3600));
}

void MI_CHAMBER_FILTER_TIME_USED::Loop() {
    if (!is_focused()) {
        set_value(config_store().chamber_filter_time_used_s.get() / 3600);
    }
}

// MI_CHAMBER_CHANGE_FILTER
// ============================================
MI_CHAMBER_CHANGE_FILTER::MI_CHAMBER_CHANGE_FILTER()
    : IWindowMenuItem(_("Change Filter")) {}

void MI_CHAMBER_CHANGE_FILTER::click(IWindowMenu &) {
#if XL_ENCLOSURE_SUPPORT()
    Screens::Access()->Open(ScreenFactory::Screen<ScreenChangeFilter>);
#else
    if (MsgBoxQuestion(_("Reset filter usage?"), Responses_YesNo) != Response::Yes) {
        return;
    }
    chamber_filtration().change_filter();
#endif
}
