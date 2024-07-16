#include "screen_filament_detail.hpp"

#include <filament_list.hpp>
#include <numeric_input_config_common.hpp>
#include <algorithm_extensions.hpp>

using namespace screen_filament_detail;

template <typename T>
concept CMI_COMMON = requires(T a, FilamentType ft) {
    { a.set_filament_type(ft) };
};

// * MI_FILAMENT_NAME
static_assert(UpdatableMenuItem<MI_FILAMENT_NAME>);

MI_FILAMENT_NAME::MI_FILAMENT_NAME()
    : MI_COMMON(_("Name"), nullptr, is_enabled_t(filament_type.is_customizable())) {}

void MI_FILAMENT_NAME::update() {
    ChangeInformation(filament_type.parameters().name);
    set_is_enabled(filament_type.is_customizable());
}

// * MI_FILAMENT_NOZZLE_TEMPERATURE
static_assert(UpdatableMenuItem<MI_FILAMENT_NOZZLE_TEMPERATURE>);

MI_FILAMENT_NOZZLE_TEMPERATURE::MI_FILAMENT_NOZZLE_TEMPERATURE()
    : MI_COMMON(0, numeric_input_config::filament_nozzle_temperature, _("Nozzle Temperature")) {}

void MI_FILAMENT_NOZZLE_TEMPERATURE::update() {
    set_value(filament_type.parameters().nozzle_temperature);
    set_is_enabled(filament_type.is_customizable());
}

// * MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE
static_assert(UpdatableMenuItem<MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE>);

MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE::MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE()
    : MI_COMMON(0, numeric_input_config::nozzle_temperature, _("Nozzle Preheat Temperature")) {}

void MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE::update() {
    set_value(filament_type.parameters().nozzle_preheat_temperature);
    set_is_enabled(filament_type.is_customizable());
}

// * MI_FILAMENT_BED_TEMPERATURE
static_assert(UpdatableMenuItem<MI_FILAMENT_BED_TEMPERATURE>);

MI_FILAMENT_BED_TEMPERATURE::MI_FILAMENT_BED_TEMPERATURE()
    : MI_COMMON(0, numeric_input_config::bed_temperature, _("Bed Temperature")) {}

void MI_FILAMENT_BED_TEMPERATURE::update() {
    set_value(filament_type.parameters().heatbed_temperature);
    set_is_enabled(filament_type.is_customizable());
}

// * MI_FILAMENT_REQUIRES_FILTRATION
static_assert(UpdatableMenuItem<MI_FILAMENT_REQUIRES_FILTRATION>);

MI_FILAMENT_REQUIRES_FILTRATION::MI_FILAMENT_REQUIRES_FILTRATION()
    : MI_COMMON(false, _("Requires Filtration")) {}

void MI_FILAMENT_REQUIRES_FILTRATION::update() {
    set_value(filament_type.parameters().requires_filtration, false);
    set_is_enabled(filament_type.is_customizable());
}

// * MI_FILAMENT_VISIBLE
static_assert(UpdatableMenuItem<MI_FILAMENT_VISIBLE>);

MI_FILAMENT_VISIBLE::MI_FILAMENT_VISIBLE()
    : MI_COMMON(false, _("Visible")) {
}

void MI_FILAMENT_VISIBLE::update() {
    set_value(filament_type.is_visible(), false);
}

void MI_FILAMENT_VISIBLE::OnChange(size_t) {
    filament_type.set_visible(value());
}

// * ScreenFilamentDetail
ScreenFilamentDetail::ScreenFilamentDetail(FilamentType filament_type)
    : ScreenMenu(_("FILAMENT DETAIL")) {

    stdext::visit_tuple(container.menu_items, [&]<typename T>(T &item) {
        if constexpr (CMI_COMMON<T>) {
            item.set_filament_type(filament_type);
        };
    });
}
