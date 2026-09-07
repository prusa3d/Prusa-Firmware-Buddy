#include "screen_filament_detail.hpp"

#include <filament_list.hpp>
#include <filament_gui.hpp>
#include <numeric_input_config_common.hpp>
#include <algorithm_extensions.hpp>
#include <dialog_text_input.hpp>
#include <ScreenHandler.hpp>
#include <utils/string_builder.hpp>
#include <screen/screen_preheat.hpp>
#include <bsod/bsod.h>

#if HAS_CHAMBER_API()
    #include <feature/chamber/chamber.hpp>
#endif

using namespace buddy;
using namespace screen_filament_detail;

// * MI_FILAMENT_NAME
MI_FILAMENT_NAME::MI_FILAMENT_NAME()
    : WiInfo(_("Name")) {}

void MI_FILAMENT_NAME::set_filament_type(FilamentType set) {
    if (filament_type_ != set) {
        filament_type_ = set;
        value_ = set.parameters().name;
        update_text();
    }
}

void MI_FILAMENT_NAME::set_value(const FilamentTypeParameters::Name &set) {
    if (value_ != set) {
        value_ = set;
        update_text();
    }
}

void MI_FILAMENT_NAME::update_text() {
    ArrayStringBuilder<GetInfoLen()> sb;
    filament_type_.build_name_with_info(value_, sb);
    ChangeInformation(sb.str());
}

void MI_FILAMENT_NAME::click(IWindowMenu &) {
    while (true) {
        if (!DialogTextInput::exec(GetLabel(), value_.data_)) {
            return;
        }

        for (char &ch : value_) {
            ch = toupper(ch);
        }

        if (const auto r = filament_type_.can_be_renamed_to(value_.data()); !r) {
            MsgBoxWarning(_(r.error()), Responses_Ok);
            continue;
        }

        break;
    }

    update_text();
}

#if HAS_FILAMENT_BASE_PRESET_PARAM()

MI_FILAMENT_BASE_PRESET::MI_FILAMENT_BASE_PRESET()
    : MenuItemSelectMenu(_("Base Preset")) {
}

MI_FILAMENT_BASE_PRESET::T MI_FILAMENT_BASE_PRESET::value() const {
    const auto i = current_item();
    debug_assert(i >= 0);
    return i == 0 ? T(std::nullopt) : static_cast<PresetFilamentType>(i - 1);
}

void MI_FILAMENT_BASE_PRESET::set_value(T set) {
    set_current_item(set.has_value() ? static_cast<int>(set.value()) + 1 : 0);
}

int MI_FILAMENT_BASE_PRESET::item_count() const {
    return static_cast<int>(PresetFilamentType::_count) + 1; // + "None"
}

string_view_utf8 MI_FILAMENT_BASE_PRESET::build_item_text(int index, ItemTextParams &) const {
    if (index == 0) {
        return _("None");
    } else {
        return string_view_utf8::MakeCPUFLASH(preset_filament_parameters[index - 1].name);
    }
}

#endif

// * MI_FILAMENT_NOZZLE_TEMPERATURE
// A filament preset is not tool-specific, so allow the range of any installed hotend (AllTools).
MI_FILAMENT_NOZZLE_TEMPERATURE::MI_FILAMENT_NOZZLE_TEMPERATURE()
    : NumericInputConfigHolder { numeric_input_config::filament_nozzle_temperature(AllTools {}) }
    , WiSpin(0, NumericInputConfigHolder::owned_config, HAS_MINI_DISPLAY() ? _("Nozzle Temp") : _("Nozzle Temperature")) {}

// * MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE
MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE::MI_FILAMENT_NOZZLE_PREHEAT_TEMPERATURE()
    : NumericInputConfigHolder { numeric_input_config::nozzle_temperature(AllTools {}) }
    , WiSpin(0, NumericInputConfigHolder::owned_config, HAS_MINI_DISPLAY() ? _("Preheat Temp") : _("Nozzle Preheat Temperature")) {}

// * MI_FILAMENT_BED_TEMPERATURE
MI_FILAMENT_BED_TEMPERATURE::MI_FILAMENT_BED_TEMPERATURE()
    : WiSpin(0, numeric_input_config::bed_temperature, HAS_MINI_DISPLAY() ? _("Bed Temp") : _("Bed Temperature")) {}

#if HAS_FILAMENT_HEATBREAK_PARAM()
MI_FILAMENT_HEATBREAK_TEMPERATURE::MI_FILAMENT_HEATBREAK_TEMPERATURE()
    : WiSpin(0, numeric_input_config::heatbreak_temperature, HAS_MINI_DISPLAY() ? _("Heatbreak Temp") : _("Heatbreak Temperature")) {}
#endif

#if HAS_CHAMBER_API()
// * MI_FILAMENT_MIN_CHAMBER_TEMPERATURE
MI_FILAMENT_MIN_CHAMBER_TEMPERATURE::MI_FILAMENT_MIN_CHAMBER_TEMPERATURE()
    : WiSpin(0, numeric_input_config::chamber_temp_with_none(), _("Minimum Chamber Temperature")) {
}

// * MI_FILAMENT_MAX_CHAMBER_TEMPERATURE
MI_FILAMENT_MAX_CHAMBER_TEMPERATURE::MI_FILAMENT_MAX_CHAMBER_TEMPERATURE()
    : WiSpin(0, numeric_input_config::chamber_temp_with_none(), _("Maximum Chamber Temperature")) {
}

// * MI_FILAMENT_TARGET_CHAMBER_TEMPERATURE
MI_FILAMENT_TARGET_CHAMBER_TEMPERATURE::MI_FILAMENT_TARGET_CHAMBER_TEMPERATURE()
    : WiSpin(0, numeric_input_config::chamber_temp_with_none(), _("Nominal Chamber Temperature")) {
}
#endif

// * MI_FILAMENT_REQUIRES_FILTRATION
#if HAS_CHAMBER_FILTRATION_API()
MI_FILAMENT_REQUIRES_FILTRATION::MI_FILAMENT_REQUIRES_FILTRATION()
    : WI_ICON_SWITCH_OFF_ON_t(false, _("Requires Filtration")) {}

#endif

// * MI_FILAMENT_ASSIGNED_OPENPRINTTAG
#if HAS_ANFC()
MI_FILAMENT_ASSIGNED_OPENPRINTTAG::MI_FILAMENT_ASSIGNED_OPENPRINTTAG()
    : WI_ICON_SWITCH_OFF_ON_t(false, _("OpenPrintTag Linked"), &img::openprinttag_white_16x16) {}

void MI_FILAMENT_ASSIGNED_OPENPRINTTAG::set_value(Value set) {
    original_uid_hash_ = set;
    WI_ICON_SWITCH_OFF_ON_t::set_value(set != no_tag_hash);
}

void MI_FILAMENT_ASSIGNED_OPENPRINTTAG::OnChange(size_t) {
    if (WI_ICON_SWITCH_OFF_ON_t::value()) {
        if (original_uid_hash_ == no_tag_hash) {
            MsgBoxError(_("OpenPrintTag must be assigned during filament load."), Responses_Ok);

            // Set the switch value to false,
            // but keep the original_uid_hash_, so that the user can reenable the assign while we still remember the hash value
            WI_ICON_SWITCH_OFF_ON_t::set_value(false);

        } else {
            // We still remember the original_uid_hash_, allow re-enabling
        }

    } else {
        const auto r = MsgBoxWarning(_("Unlink OpenPrintTag from the filament? Filament usage tracking will be disabled."), Responses_YesNo);
        if (r != Response::Yes) {
            WI_ICON_SWITCH_OFF_ON_t::set_value(true);
        }
    }
}
#endif

// * MI_FILAMENT_IS_ABRASIVE
MI_FILAMENT_IS_ABRASIVE::MI_FILAMENT_IS_ABRASIVE()
    : WI_ICON_SWITCH_OFF_ON_t(false, _("Is Abrasive")) {}

// * MI_FILAMENT_IS_FLEXIBLE
MI_FILAMENT_IS_FLEXIBLE::MI_FILAMENT_IS_FLEXIBLE()
    : WI_ICON_SWITCH_OFF_ON_t(false, _("Is Flexible")) {}

// * MI_FILAMENT_VISIBLE
MI_FILAMENT_VISIBLE::MI_FILAMENT_VISIBLE()
    : WI_ICON_SWITCH_OFF_ON_t(false, _("Visible")) {
}

// * MI_CONFIRM
MI_CONFIRM::MI_CONFIRM()
    : IWindowMenuItem(_("Confirm"), &img::ok_16x16) {}

void MI_CONFIRM::click(IWindowMenu &) {
    callback();
}

// * ScreenFilamentDetail
ScreenFilamentDetail::ScreenFilamentDetail(FilamentType filament_type)
    : ScreenFilamentDetail(N_("FILAMENT DETAIL")) {
    setup(filament_type);
}

ScreenFilamentDetail::ScreenFilamentDetail(PreheatModeParams params)
    : ScreenFilamentDetail(N_("CUSTOM PARAMETERS")) {

    setup(PendingAdHocFilamentType {});
    setup_preheat_mode_confirm(params);
}

ScreenFilamentDetail::ScreenFilamentDetail(const char *title)
    : ScreenMenu(_(title)) {

    Item<MI_CONFIRM>().set_is_hidden();
}

ScreenFilamentDetail::~ScreenFilamentDetail() {
    if (filament_type_ != FilamentType::none) {
        save_changes();
    }
}

void ScreenFilamentDetail::setup(FilamentType filament_type, const FilamentTypeParameters &params) {
    filament_type_ = filament_type;

    const bool is_customizable = filament_type.is_customizable();

#if HAS_CHAMBER_API()
    const auto chamber_caps = chamber().capabilities();
    const bool hide_chamber_items = !chamber_caps.temperature_control() && !chamber_caps.always_show_temperature_control;
#endif

    Item<MI_FILAMENT_NAME>().set_filament_type(filament_type);

    stdext::visit_tuple(container.menu_items, [&]<typename T>(T &item) {
        static constexpr bool is_utility_button = std::is_same_v<T, MI_RETURN> || std::is_same_v<T, MI_CONFIRM>;

        if constexpr (!is_utility_button) {
            item.set_enabled(is_customizable);
        };

#if HAS_CHAMBER_API()
        if constexpr (requires { T::is_chamber_item; }) {
            item.set_is_hidden(hide_chamber_items);
        }
#endif

#if HAS_FILAMENT_BASE_PRESET_PARAM()
        if constexpr (std::is_same_v<T, MI_FILAMENT_BASE_PRESET>) {
            // Presets have the base_preset set as identity. No point in showing it
            item.set_is_hidden(std::holds_alternative<PresetFilamentType>(filament_type));
        }
#endif

        if constexpr (is_utility_button) {
            // pass

        } else if constexpr (std::is_same_v<T, MI_FILAMENT_VISIBLE>) {
            item.set_is_hidden(!filament_type.is_visibility_customizable());
            item.set_value(filament_type.is_visible());

        } else if constexpr (true) {
            item.set_value(params.*(T::parameter_ptr));
        }
    });
}

void ScreenFilamentDetail::save_changes() {
    // Apply the new parameters ()
    FilamentTypeParameters params;

    stdext::visit_tuple(container.menu_items, [&]<typename T>(T &item) {
        static constexpr bool is_utility_button = std::is_same_v<T, MI_RETURN> || std::is_same_v<T, MI_CONFIRM>;

        if constexpr (is_utility_button) {
            // pass

        } else if constexpr (std::is_same_v<T, MI_FILAMENT_VISIBLE>) {
            if (!item.IsHidden()) {
                // Only update visibility if the user had the chance to set it (= if the visibility is customizable)
                filament_type_.set_visible(item.value());
            }

        } else if constexpr (true) {
            // Note: the static_cast is needed because of WiSpin which always works with floats storing to int16_t temperatures
            params.*(T::parameter_ptr) = static_cast<std::remove_cvref_t<decltype(params.*(T::parameter_ptr))>>(item.value());
        }
    });

    if (filament_type_.is_customizable()) {
        filament_type_.set_parameters(params);
    }
}

void ScreenFilamentDetail::setup(FilamentType filament_type) {
    setup(filament_type, filament_type.parameters());
}

void screen_filament_detail::ScreenFilamentDetail::setup_preheat_mode_confirm(PreheatModeParams params) {
    auto &confirm_item = Item<MI_CONFIRM>();
    confirm_item.set_is_hidden(false);
    confirm_item.callback = [this, params] {
        // handle_filament_selection is reading from the filament parameters for checking, so we need to update them
        save_changes();

        if (ScreenPreheat::handle_filament_selection(PendingAdHocFilamentType {}, params.tool, params.mode)) {
            Screens::Access()->Close();
        }
    };
}
