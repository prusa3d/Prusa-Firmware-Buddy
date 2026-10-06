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
    : WiInfo(_("Short Name")) {}

void MI_FILAMENT_NAME::set_params(const BuildFilamentNameWithInfoParams &set) {
    if (data_ == set) {
        return;
    }

    data_ = set;
    update_text();
}

void MI_FILAMENT_NAME::set_value(const FilamentTypeParameters::Name &set) {
    if (value() == set) {
        return;
    }

    data_.name = set;
    update_text();
}

void MI_FILAMENT_NAME::update_text() {
    ArrayStringBuilder<GetInfoLen()> sb;
    FilamentType::build_name_with_info(data_, sb);
    ChangeInformation(sb.str());
}

void MI_FILAMENT_NAME::click(IWindowMenu &) {
    FilamentTypeParameters::Name new_name = value();

    while (true) {
        if (!DialogTextInput::exec(GetLabel(), new_name.data_)) {
            return;
        }

        for (char &ch : new_name) {
            ch = toupper(ch);
        }

        if (const auto r = data_.type.can_be_renamed_to(new_name); !r) {
            MsgBoxWarning(_(r.error()), Responses_Ok);
            continue;
        }

        break;
    }

    set_value(new_name);
}

//* MI_FILAMENT_COLOR
MI_FILAMENT_COLOR::MI_FILAMENT_COLOR()
    : MenuItemColor(_("Color")) {
}

#if HAS_FILAMENT_BASE_PRESET_PARAM()

MI_FILAMENT_BASE_PRESET::MI_FILAMENT_BASE_PRESET()
    : MenuItemSelectMenu(_("Base Preset")) {
}

MI_FILAMENT_BASE_PRESET::T MI_FILAMENT_BASE_PRESET::value() const {
    const auto i = current_item();
    debug_assert(i >= 0);
    return i == 0 ? T(std::nullopt) : preset_filament_types[i - 1];
}

void MI_FILAMENT_BASE_PRESET::set_value(T set) {
    int new_item_index = 0;
    if (set.has_value()) {
        if (auto ix = stdext::index_of_opt(preset_filament_types, *set)) {
            new_item_index = *ix + 1;
        }
    }

    set_current_item(new_item_index);
}

int MI_FILAMENT_BASE_PRESET::item_count() const {
    return preset_filament_types.size() + 1; // + "None"
}

string_view_utf8 MI_FILAMENT_BASE_PRESET::build_item_text(int index, StringViewUtf8ParamBase &params) const {
    if (index == 0) {
        return _("None");
    } else {
        return string_view_utf8::MakeCPUFLASH("%.*s").formatted(params, std::string_view(preset_filament_parameters(preset_filament_types[index - 1]).name));
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

// * ScreenFilamentDetail
ScreenFilamentDetail::ScreenFilamentDetail(FilamentType filament_type)
    : ScreenFilamentDetail(N_("FILAMENT DETAIL")) {
    setup(filament_type, SetupArgs {});
}

ScreenFilamentDetail::ScreenFilamentDetail(PreheatModeParams params)
    : ScreenFilamentDetail(N_("CUSTOM PARAMETERS")) {

    setup(PendingAdHocFilamentType {}, SetupArgs {});
    setup_preheat_mode_confirm(params);
}

ScreenFilamentDetail::ScreenFilamentDetail(LoadedFilamentMode loaded_filament_mode)
    : ScreenFilamentDetail(nullptr) {

    const VirtualToolIndex tool = loaded_filament_mode.tool;
    header.SetText(_("%c%i LOADED FILAMENT").formatted(title_params_, tool.compact_name_prefix_char, tool.display_index()));

    filament_type_ = FilamentType::for_tool(tool);
    if (!filament_type_) {
        close_screen();
        return;
    }

    const SetupArgs setup_args {
        // This is "loaded filament" info
        // Editing a non-adhoc filament type (say user preset)
        // could result in changing the parameters to other loaded slots as well.
        // Enforce cast to Adhoc filament through the "Customize" button
        .force_disable_customizable = !filament_type_.is_adhoc(),
    };
    setup(filament_type_, setup_args);

    // If the filament type is preset/user preset,
    if (!filament_type_.is_adhoc()) {
        auto &item = Item<MI_CALLBACK>();
        item.SetLabel(_("Customize"));
        item.set_is_hidden(false);
        item.callback = [this, tool] {
            const Response r = MsgBoxInfo(
                _("Customize the loaded filament? Changes will apply to this slot only and will be lost on unload."),
                { Response::Ok, Response::Cancel });
            if (r != Response::Ok) {
                return;
            }

            // Transfer to the new filament type
            const FilamentType new_filament_type = AdHocFilamentType { .tool = tool };
            const FilamentTypeParameters params = filament_type_.parameters();
            new_filament_type.set_parameters(params);
            config_store().set_filament_type(tool, new_filament_type);

            // Silence the asserts; we are changing set_is_hidden after the menu has been first drawn
            // which we shouldn't generally do because the system is not ready for that
            // But here we (hopefully) know what we're doing
            for (int i = 0; i < menu.menu.item_count(); i++) {
                menu.menu.item_at(i)->Invalidate();
            }

            // Re-setup the GUI
            filament_type_ = new_filament_type;
            menu.menu.move_focus_to_index(std::nullopt);
            menu.menu.set_scroll_offset(0);
            setup(filament_type_, params, SetupArgs {});
            Item<MI_CALLBACK>().set_is_hidden(true);
            Invalidate();
        };
    }
}

ScreenFilamentDetail::ScreenFilamentDetail(const char *title)
    : ScreenMenu(_(title)) {
    Item<MI_CALLBACK>().set_is_hidden();
}

ScreenFilamentDetail::~ScreenFilamentDetail() {
    if (filament_type_ != FilamentType::none) {
        save_changes();
    }
}

void ScreenFilamentDetail::setup(FilamentType filament_type, SetupArgs args) {
    setup(filament_type, filament_type.parameters(), args);
}

void ScreenFilamentDetail::setup(FilamentType filament_type, const FilamentTypeParameters &params, SetupArgs args) {
    filament_type_ = filament_type;

    const bool is_customizable = filament_type.is_customizable() && !args.force_disable_customizable;

#if HAS_CHAMBER_API()
    const bool hide_chamber_items = !chamber().capabilities().temperature_control_generally();
#endif

    Item<MI_FILAMENT_NAME>().set_params(BuildFilamentNameWithInfoParams(filament_type, params));

    stdext::visit_tuple(container.menu_items, [&]<typename T>(T &item) {
        static constexpr bool is_utility_button = ::is_utility_button<T>;

        bool set_hidden = false;

        if constexpr (requires { T::is_adhoc_only; }) {
            set_hidden |= !(
                filament_type.is_adhoc()
                // Special case when showing data from OPT without any filament linkage
                // See ScreenOPTFilamentDetail
                || std::holds_alternative<NoFilamentType>(filament_type));
        }

#if HAS_ANFC()
        if constexpr (std::is_same_v<T, MI_FILAMENT_ASSIGNED_OPENPRINTTAG>) {
            // MI_FILAMENT_ASSIGNED_OPENPRINTTAG doesn't have the NoFilament exception
            // When showing data from OPT directly, linkage information is meaningless
            set_hidden |= !filament_type.is_adhoc();
        }
#endif

#if HAS_CHAMBER_API()
        if constexpr (requires { T::is_chamber_item; }) {
            set_hidden |= hide_chamber_items;
        }
#endif

#if HAS_FILAMENT_BASE_PRESET_PARAM()
        if constexpr (std::is_same_v<T, MI_FILAMENT_BASE_PRESET>) {
            // Presets have the base_preset set as identity. No point in showing it
            set_hidden |= std::holds_alternative<PresetFilamentType>(filament_type);
        }
#endif

        if constexpr (is_utility_button) {
            // pass

        } else if constexpr (std::is_same_v<T, MI_FILAMENT_VISIBLE>) {
            set_hidden |= !filament_type.is_visibility_customizable();
            item.set_value(filament_type.is_visible());

        } else if constexpr (true) {
            item.set_value(params.*(T::parameter_ptr));
            item.set_enabled(is_customizable);
        }

        if constexpr (!is_utility_button) {
            // Utility buttons have visibility managed separately
            item.set_is_hidden(set_hidden);
        }
    });
}

void ScreenFilamentDetail::save_changes() {
    // Apply the new parameters ()
    FilamentTypeParameters params;

    stdext::visit_tuple(container.menu_items, [&]<typename T>(T &item) {
        static constexpr bool is_utility_button = std::is_same_v<T, MI_RETURN> || std::is_same_v<T, MI_CALLBACK>;

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

void ScreenFilamentDetail::setup_preheat_mode_confirm(PreheatModeParams params) {
    auto &confirm_item = Item<MI_CALLBACK>();
    confirm_item.SetLabel(_("Confirm"));
    confirm_item.SetIconId(&img::ok_16x16);
    confirm_item.set_is_hidden(false);
    confirm_item.callback = [this, params] {
        // handle_filament_selection is reading from the filament parameters for checking, so we need to update them
        save_changes();

        if (ScreenPreheat::handle_filament_selection(PendingAdHocFilamentType {}, params.tool, params.mode)) {
            close_screen();
        }
    };
}
