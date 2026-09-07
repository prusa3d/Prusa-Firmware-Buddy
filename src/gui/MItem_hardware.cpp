#include "MItem_hardware.hpp"
#include "ScreenHandler.hpp"
#include "WindowMenuSpin.hpp"
#include "window_msgbox.hpp"
#include "marlin_client.hpp"
#include <common/sys.hpp>
#include <option/has_15gt_belts.h>
#include <option/has_toolchanger.h>
#include <option/has_side_fsensor_remap.h>
#include <option/has_nozzle_cleaner_lite.h>
#include <common/nozzle_diameter.hpp>
#include <common/printer_model_data.hpp>
#include <common/extended_printer_type.hpp>
#include <option/has_print_fan_type.h>

#if HAS_CHAMBER_VENTS()
    #include <feature/chamber/chamber_enums.hpp>
#endif

#include <option/has_side_fsensor_remap.h>
#if HAS_SIDE_FSENSOR_REMAP()
    #include <feature/filament_sensor/filament_sensors_handler_remap.hpp>
#endif

static constexpr const char *hw_check_items[] = {
    N_("None"),
    N_("Warn"),
    N_("Strict"),
};

MI_HARDWARE_CHECK::MI_HARDWARE_CHECK(HWCheckType check_type)
    : MenuItemSwitch(_(hw_check_type_names[check_type]), hw_check_items, static_cast<int>(config_store().visit_hw_check(check_type, [](auto &item) { return item.get(); })))
    , check_type(check_type) //
{}

void MI_HARDWARE_CHECK::OnChange([[maybe_unused]] size_t old_index) {
    config_store().visit_hw_check(check_type, [set = static_cast<HWCheckSeverity>(this->get_index())](auto &item) { item.set(set); });
}

#if HAS_SIDE_FSENSOR_REMAP()
// MI_SIDE_FSENSOR_REMAP
MI_SIDE_FSENSOR_REMAP::MI_SIDE_FSENSOR_REMAP()
    : WI_ICON_SWITCH_OFF_ON_t(side_fsensor_remap::is_remapped(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {};

void MI_SIDE_FSENSOR_REMAP::OnChange([[maybe_unused]] size_t old_index) {
    if (uint8_t mask = side_fsensor_remap::ask_to_remap(); mask != 0) { // Ask user to remap
        Screens::Access()->Get()->Validate(); // Do not redraw this menu yet

        // Change index by what user selected)
        set_value(side_fsensor_remap::is_remapped());

    #if HAS_SELFTEST()
        Validate(); // Do not redraw this switch yet
        marlin_client::gcode_printf("M1981 F%i", (int)mask); // Start filament sensor calibration for moved tools
    #endif

    } else {
        // Change index by what user selected)
        set_value(side_fsensor_remap::is_remapped());
    }
}
#endif

#if IS_EXTENDED_PRINTER_TYPE_CONFIGURABLE()
MI_EXTENDED_PRINTER_TYPE::MI_EXTENDED_PRINTER_TYPE()
    : MenuItemSelectMenu(_("Printer Type")) //
{
    set_current_item(config_store().extended_printer_type.get());
}

int MI_EXTENDED_PRINTER_TYPE::item_count() const {
    return static_cast<int>(extended_printer_type_model.size());
}

string_view_utf8 MI_EXTENDED_PRINTER_TYPE::build_item_text(int index, [[maybe_unused]] MenuItemSelectMenu::ItemTextParams &params) const {
    return string_view_utf8::MakeCPUFLASH(PrinterModelInfo::get(extended_printer_type_model[index]).display_str());
}

bool MI_EXTENDED_PRINTER_TYPE::on_item_selected(const OnItemSelectedArgs &args) {
    change_extended_printer_type(extended_printer_type_model[args.new_index], ChangeExtendedPrinterTypeMode::standard_with_marlin_client_and_puppies);
    return true;
}
#endif

#if HAS_PRINTER_VARIANT()
MI_PRINTER_VARIANT::MI_PRINTER_VARIANT()
    : MenuItemSelectMenu(_("Edition")) {
    // The selection is derived from the feature flags in Loop(), which the menu fires right after creation.
}

void MI_PRINTER_VARIANT::Loop() {
    // The current edition is derived from the feature flags (config store is the source of truth).
    const auto current = printer_variant_from_config();
    // Flags match no edition (user override) -> show a trailing, display-only "Custom" row.
    index_mapping.set_item_enabled<Item::custom>(!current.has_value());
    if (current) {
        set_current_item(index_mapping.to_index<Item::variant>(std::to_underlying(*current)));
    } else {
        set_current_item(index_mapping.to_index<Item::custom>());
    }
}

int MI_PRINTER_VARIANT::item_count() const {
    return index_mapping.total_item_count();
}

string_view_utf8 MI_PRINTER_VARIANT::build_item_text(int index, [[maybe_unused]] MenuItemSelectMenu::ItemTextParams &params) const {
    const auto mapping = index_mapping.from_index(index);
    switch (mapping.item) {

    case Item::variant:
        return string_view_utf8::MakeCPUFLASH(printer_variant_names[mapping.pos_in_section]);

    case Item::custom:
        return _("Custom");
    }

    bsod_unreachable();
}

bool MI_PRINTER_VARIANT::on_item_selected(const OnItemSelectedArgs &args) {
    const auto mapping = index_mapping.from_index(args.new_index);
    if (mapping.item != Item::variant) {
        return false; // "Custom" is display-only, there is no preset to apply
    }
    const auto variant = static_cast<PrinterVariant>(mapping.pos_in_section);

    if (MsgBoxWarning(_("Selecting an edition applies that edition's default hardware options, overriding any manual changes. Continue?"),
            { Response::Yes, Response::No }, 1)
        != Response::Yes) {
        return false; // cancelled -> MenuItemSelectMenu keeps the previous edition selected
    }

    if (apply_printer_variant_defaults(variant)) {
        sys_reset();
    }
    return true;
}
#endif

#if HAS_EMERGENCY_STOP()
static bool user_made_informed_decision_to_disable_door_sensor() {
    const Response response = MsgBoxWarning(
        _(
            "Caution! Disabling the door sensor may lead to injury or printer damage. "
            "Proceeding means you accept full responsibility. "
            "We are not liable for any harm or damages."),
        { Response::Disable, Response::Cancel },
        1 /* default is to cancel in order to prevent double clicks */);
    return response == Response::Disable;
}

MI_EMERGENCY_STOP_ENABLE::MI_EMERGENCY_STOP_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().emergency_stop_enable.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {};

void MI_EMERGENCY_STOP_ENABLE::OnChange([[maybe_unused]] size_t old_index) {
    if (!value() && !user_made_informed_decision_to_disable_door_sensor()) {
        // revert the change in GUI and keep config store intact
        set_value(true);
        return;
    }

    config_store().emergency_stop_enable.set(value());
    config_store().emergency_stop_disable_consent_given.set(!value());
}
#endif

#if HAS_CHAMBER_VENTS()
static constexpr const char *chamber_vent_control_items[] = {
    N_("Off"),
    N_("Auto"),
    N_("Manual"),
};

static_assert(VentControl(0) == VentControl::off && VentControl(1) == VentControl::automatic && VentControl(2) == VentControl::manual, "menu item misalignment");

MI_SWITCH_VENT_MECHANISM::MI_SWITCH_VENT_MECHANISM()
    : MenuItemSwitch(_("Chamber Vent Control"), chamber_vent_control_items, 0) {}

void MI_SWITCH_VENT_MECHANISM::OnChange([[maybe_unused]] size_t old_index) {
    config_store().set_vent_control(VentControl(get_index()));
}

void MI_SWITCH_VENT_MECHANISM::Loop() {
    set_current_item(std::to_underlying(config_store().get_vent_control()));
}
#endif

#if HAS_SWITCHABLE_HOMING_CALIBRATION()
constexpr const EnumArray<Tristate::Value, const char *, 3> ask_always_never_texts {
    { Tristate::no, N_("Never") },
    { Tristate::yes, N_("Auto") },
    { Tristate::other, N_("Ask") },
};

MI_AUTO_PRECISE_HOMING_CALIBRATION::MI_AUTO_PRECISE_HOMING_CALIBRATION()
    : MenuItemSwitch(_("Homing Calibration"), ask_always_never_texts, config_store().auto_recalibrate_precise_homing.get().value) {
}

void MI_AUTO_PRECISE_HOMING_CALIBRATION::OnChange(size_t) {
    config_store().auto_recalibrate_precise_homing.set(static_cast<Tristate::Value>(get_index()));
}
#endif
#if HAS_EXPANSION_JOINTS_GEN_2()
MI_EXPANSION_JOINTS_GEN_2::MI_EXPANSION_JOINTS_GEN_2()
    : WI_ICON_SWITCH_OFF_ON_t(false, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_EXPANSION_JOINTS_GEN_2::OnChange([[maybe_unused]] size_t old_index) {
    config_store().ejg2_installed.set(value());
}

void MI_EXPANSION_JOINTS_GEN_2::Loop() {
    if (const bool installed = config_store().ejg2_installed.get(); value() != installed) {
        set_value(installed);
    }
}
#endif

#if HAS_NOZZLE_CLEANER_LITE()
MI_NOZZLE_CLEANER_LITE::MI_NOZZLE_CLEANER_LITE()
    : WI_ICON_SWITCH_OFF_ON_t(false, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {};

void MI_NOZZLE_CLEANER_LITE::OnChange([[maybe_unused]] size_t old_index) {
    config_store().nozzle_cleaner_lite_installed.set(value());
}

void MI_NOZZLE_CLEANER_LITE::Loop() {
    if (const bool present = config_store().nozzle_cleaner_lite_installed.get(); value() != present) {
        set_value(present);
    }
}
#endif

#if HAS_15GT_BELTS()
MI_BELTS_15GT::MI_BELTS_15GT()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().belts_15gt_installed.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_BELTS_15GT::OnChange([[maybe_unused]] size_t old_index) {
    const bool belts_15gt_installed = value();
    if (MsgBoxWarning(_("Changing belt type updates X/Y steps/mm, and resets some calibrations. An incorrect setting causes dimensional errors and homing issues. Continue?"),
            { Response::Yes, Response::No }, 1)
        != Response::Yes) {
        set_value(!belts_15gt_installed); // revert the GUI, keep config store intact
        return;
    }

    if (config_store().set_belts_15gt(belts_15gt_installed)) {
        marlin_client::gcode_printf("M92 X%f Y%f", (double)get_steps_per_unit_x(), (double)get_steps_per_unit_y());
    }
}
#endif
