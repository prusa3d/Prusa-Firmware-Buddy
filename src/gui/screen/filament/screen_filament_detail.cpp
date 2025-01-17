#include "screen_filament_detail.hpp"

#include <filament_list.hpp>
#include <filament_gui.hpp>
#include <numeric_input_config_common.hpp>
#include <algorithm_extensions.hpp>
#include <dialog_text_input.hpp>
#include <ScreenHandler.hpp>

using namespace screen_filament_detail;

// * MI_FILAMENT_NAME
MI_FILAMENT_NAME::MI_FILAMENT_NAME()
    : MI_COMMON(_("Name"), nullptr, is_enabled_t(filament_type.is_customizable())) {}

void MI_FILAMENT_NAME::update() {
    ArrayStringBuilder<GetInfoLen()> sb;
    filament_type.build_name_with_info(sb);
    ChangeInformation(sb.str());
    set_enabled(filament_type.is_customizable());
}

void MI_FILAMENT_NAME::click(IWindowMenu &) {
    FilamentTypeParameters::Name buf = filament_type.parameters().name;

    while (true) {
        if (!DialogTextInput::exec(GetLabel(), buf)) {
            return;
        }

        for (char *chp = buf.data(); *chp; chp++) {
            *chp = toupper(*chp);
        }

        if (const auto r = filament_type.can_be_renamed_to(buf.data()); !r) {
            MsgBoxWarning(_(r.error()), Responses_Ok);
            continue;
        }

        break;
    }

    filament_type.modify_parameters([&](auto &p) { memcpy(p.name.data(), buf.data(), buf.size()); });
    update();
}

// * MI_FILAMENT_NOZZLE_TEMPERATURE
MI_FILAMENT_NOZZLE_TEMPERATURE::MI_FILAMENT_NOZZLE_TEMPERATURE()
    : MI_SPIN(&FilamentTypeParameters::nozzle_temperature, numeric_input_config::filament_nozzle_temperature, HAS_MINI_DISPLAY() ? N_("Nozzle Temp") : N_("Nozzle Temperature")) {}

// * MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE
MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE::MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE()
    : MI_SPIN(&FilamentTypeParameters::nozzle_preheat_temperature, numeric_input_config::nozzle_temperature, HAS_MINI_DISPLAY() ? N_("Preheat Temp") : N_("Nozzle Preheat Temperature")) {}

// * MI_FILAMENT_BED_TEMPERATURE
MI_FILAMENT_BED_TEMPERATURE::MI_FILAMENT_BED_TEMPERATURE()
    : MI_SPIN(&FilamentTypeParameters::heatbed_temperature, numeric_input_config::bed_temperature, HAS_MINI_DISPLAY() ? N_("Bed Temp") : N_("Bed Temperature")) {}

// * MI_FILAMENT_REQUIRES_FILTRATION
#if HAS_CHAMBER_API()
MI_FILAMENT_REQUIRES_FILTRATION::MI_FILAMENT_REQUIRES_FILTRATION()
    : MI_COMMON(false, _("Requires Filtration")) {}

void MI_FILAMENT_REQUIRES_FILTRATION::update() {
    set_value(filament_type.parameters().requires_filtration, false);
    set_enabled(filament_type.is_customizable());
}

void MI_FILAMENT_REQUIRES_FILTRATION::OnChange(size_t) {
    filament_type.modify_parameters([&](auto &p) { p.requires_filtration = value(); });
}
#endif

// * MI_FILAMENT_IS_ABRASIVE
MI_FILAMENT_IS_ABRASIVE::MI_FILAMENT_IS_ABRASIVE()
    : MI_COMMON(false, _("Is Abrasive")) {}

void MI_FILAMENT_IS_ABRASIVE::update() {
    set_value(filament_type.parameters().is_abrasive, false);
    set_enabled(filament_type.is_customizable());
}

void MI_FILAMENT_IS_ABRASIVE::OnChange(size_t) {
    filament_type.modify_parameters([&](auto &p) { p.is_abrasive = value(); });
}

// * MI_FILAMENT_VISIBLE
MI_FILAMENT_VISIBLE::MI_FILAMENT_VISIBLE()
    : MI_COMMON(false, _("Visible")) {
}

void MI_FILAMENT_VISIBLE::update() {
    set_value(filament_type.is_visible(), false);
    set_is_hidden(filament_type.is_visibility_customizable());
}

void MI_FILAMENT_VISIBLE::OnChange(size_t) {
    filament_type.set_visible(value());
}

// * MI_PREHEAT_CONFIRM
MI_PREHEAT_CONFIRM::MI_PREHEAT_CONFIRM()
    : MI_COMMON(_("Confirm"), &img::ok_16x16) {}

void MI_PREHEAT_CONFIRM::update() {
}

void MI_PREHEAT_CONFIRM::click(IWindowMenu &) {
    marlin_client::FSM_response_variant(PhasesPreheat::UserTempSelection, FSMResponseVariant::make<FilamentType>(filament_type));
    Screens::Access()->Close();
}

// * ScreenFilamentDetail
ScreenFilamentDetail::ScreenFilamentDetail(Params params)
    : ScreenMenu(params.mode == Mode::preheat ? _("CUSTOM PARAMETERS") : _("FILAMENT DETAIL")) {

    stdext::visit_tuple(container.menu_items, [&]<typename T>(T &item) {
        if constexpr (!std::is_same_v<T, MI_RETURN>) {
            item.set_filament_type(params.filament_type);
        };
    });

    Item<MI_PREHEAT_CONFIRM>().set_is_hidden(params.mode != Mode::preheat);
}
