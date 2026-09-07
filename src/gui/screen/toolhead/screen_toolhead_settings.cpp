#include "screen_toolhead_settings.hpp"

#include <common/nozzle_diameter.hpp>
#include <ScreenHandler.hpp>
#include <img_resources.hpp>
#include <gui/dialogs/window_dlg_wait.hpp>
#include <gcode/queue.h>
#include <module/planner.h>
#include <utils/string_builder.hpp>
#include <utils/variant_utils.hpp>
#include <option/has_toolchanger.h>
#include <option/has_tool_offset_sensor.h>
#include <option/has_ht_hotend.h>

#include "screen_toolhead_settings_fs.hpp"
#include "screen_toolhead_settings_dock.hpp"
#include "screen_toolhead_settings_nozzle_offset.hpp"
#include "tool_index.hpp"

using namespace screen_toolhead_settings;

static constexpr NumericInputConfig nozzle_diameter_spin_config_with_special = [] {
    NumericInputConfig result = nozzle_diameter_spin_config;
    result.special_value = 0;
    result.special_value_str = "-";
    return result;
}();

// * MI_NOZZLE_DIAMETER
MI_NOZZLE_DIAMETER::MI_NOZZLE_DIAMETER(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_SPIN(toolhead, 0, nozzle_diameter_spin_config_with_special, _("Nozzle Diameter")) {
    update();
}

float MI_NOZZLE_DIAMETER::read_value_impl(PhysicalToolIndex ix) {
    return config_store().get_nozzle_diameter(ix);
}

void MI_NOZZLE_DIAMETER::store_value_impl(PhysicalToolIndex ix, float set) {
    config_store().set_nozzle_diameter(ix, set);
}

// * MI_NOZZLE_DIAMETER_HELP
MI_NOZZLE_DIAMETER_HELP::MI_NOZZLE_DIAMETER_HELP()
    : IWindowMenuItem(_("What nozzle diameter do I have?"), &img::question_16x16) {
}

void MI_NOZZLE_DIAMETER_HELP::click(IWindowMenu &) {
    MsgBoxInfo(_("You can determine the nozzle diameter by counting the markings (dots) on the nozzle:\n"
                 "  0.40 mm nozzle: 3 dots\n"
                 "  0.60 mm nozzle: 4 dots\n\n"
                 "For more information, visit prusa.io/nozzle-types"),
        Responses_Ok);
}

#if HAS_HOTEND_TYPE_SUPPORT()
// * MI_HOTEND_TYPE
MI_HOTEND_TYPE::MI_HOTEND_TYPE(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC(toolhead, _("Hotend Type")) {
    update();
}

int MI_HOTEND_TYPE::item_count() const {
    // If has varying values, the 0th item is "-" (for different values)
    return hotend_type_list.size() + (has_varying_values_ ? 1 : 0);
}

string_view_utf8 MI_HOTEND_TYPE::build_item_text(int index, [[maybe_unused]] MenuItemSelectMenu::ItemTextParams &params) const {
    const int effective_index = index - (has_varying_values_ ? 1 : 0);

    // If has varying values, the 0th item is "-" (for different values)
    if (effective_index == -1) {
        return string_view_utf8::MakeCPUFLASH("-");
    } else {
        return _(hotend_type_name(hotend_type_list[effective_index]));
    }
}

bool MI_HOTEND_TYPE::on_item_selected(const OnItemSelectedArgs &args) {
    const int effective_index = args.new_index - (has_varying_values_ ? 1 : 0);

    if (effective_index == -1) {
        return false;
    }

    if (!msgbox_confirm_change(toolhead(), user_already_confirmed_changes_)) {
        return false;
    }

    store_value(hotend_type_list[effective_index]);
    return true;
}

void MI_HOTEND_TYPE::update() {
    const auto val = read_value();
    has_varying_values_ = !val.has_value();

    // If has varying values, the 0th item is "-" (for different values)
    // Force set - we might be changing item texts here
    force_set_current_item(has_varying_values_ ? 0 : stdext::index_of(hotend_type_list, *val));
}

HotendType MI_HOTEND_TYPE::read_value_impl(PhysicalToolIndex ix) {
    return config_store().hotend_type.get(ix.to_raw());
}

void MI_HOTEND_TYPE::store_value_impl(PhysicalToolIndex ix, HotendType set) {
    config_store().hotend_type.set(ix.to_raw(), set);
}

// * MI_NOZZLE_SOCK
MI_NOZZLE_SOCK::MI_NOZZLE_SOCK(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_TOGGLE(toolhead, false, _("Nextruder Silicone Sock")) {
    update();
}

bool MI_NOZZLE_SOCK::read_value_impl(PhysicalToolIndex ix) {
    return config_store().hotend_type.get(ix.to_raw()) == HotendType::stock_with_sock;
}

void MI_NOZZLE_SOCK::store_value_impl(PhysicalToolIndex ix, bool set) {
    config_store().hotend_type.set(ix.to_raw(), set ? HotendType::stock_with_sock : HotendType::stock);
}
#endif /* HAS_HOTEND_TYPE_SUPPORT() */

#if HAS_PRINT_FAN_TYPE()
MI_PRINT_FAN_TYPE::MI_PRINT_FAN_TYPE(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC(toolhead, _("Print Fan Type")) {
    update();
}

PrintFanType MI_PRINT_FAN_TYPE::read_value_impl(PhysicalToolIndex ix) {
    return get_print_fan_type(ix);
}

void MI_PRINT_FAN_TYPE::store_value_impl(PhysicalToolIndex ix, PrintFanType set) {
    set_print_fan_type(ix, set);
}

string_view_utf8 MI_PRINT_FAN_TYPE::build_item_text(int index, [[maybe_unused]] MenuItemSelectMenu::ItemTextParams &params) const {
    const int effective_index = index - (has_varying_values_ ? 1 : 0);

    // If has varying values, the 0th item is "-" (for different values)
    if (effective_index == -1) {
        return string_view_utf8::MakeCPUFLASH("-");
    } else {
        return _(print_fan_type_names[print_fan_type_list[effective_index]]);
    }
}

bool MI_PRINT_FAN_TYPE::on_item_selected(const OnItemSelectedArgs &args) {
    const int effective_index = args.new_index - (has_varying_values_ ? 1 : 0);

    if (effective_index == -1) {
        return false;
    }

    if (!msgbox_confirm_change(toolhead(), user_already_confirmed_changes_)) {
        return false;
    }

    store_value(print_fan_type_list[effective_index]);
    return true;
}

void MI_PRINT_FAN_TYPE::update() {
    const auto val = read_value();
    has_varying_values_ = !val.has_value();

    // If has varying values, the 0th item is "-" (for different values)
    // Force set - we might be changing item texts here
    force_set_current_item(has_varying_values_ ? 0 : stdext::index_of(print_fan_type_list, *val));
}

int MI_PRINT_FAN_TYPE::item_count() const {
    // If has varying values, the 0th item is "-" (for different values)
    return print_fan_type_list.size() + (has_varying_values_ ? 1 : 0);
}
#endif /* HAS_PRINT_FAN_TYPE */

// * MI_NOZZLE_HARDENED
MI_NOZZLE_HARDENED::MI_NOZZLE_HARDENED(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_TOGGLE(toolhead, false, _("Nozzle Hardened")) //
{
    update();
}

bool MI_NOZZLE_HARDENED::read_value_impl(PhysicalToolIndex ix) {
    return config_store().get_nozzle_is_hardened(ix);
}

void MI_NOZZLE_HARDENED::store_value_impl(PhysicalToolIndex ix, bool set) {
    config_store().set_nozzle_is_hardened(ix, set);
}

// * MI_NOZZLE_HIGH_FLOW
MI_NOZZLE_HIGH_FLOW::MI_NOZZLE_HIGH_FLOW(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_TOGGLE(toolhead, false, _("Nozzle High-flow")) //
{
    update();
}

bool MI_NOZZLE_HIGH_FLOW::read_value_impl(PhysicalToolIndex ix) {
    return config_store().get_nozzle_is_high_flow(ix);
}

void MI_NOZZLE_HIGH_FLOW::store_value_impl(PhysicalToolIndex ix, bool set) {
    config_store().set_nozzle_is_high_flow(ix, set);
}

#if HAS_TOOLCHANGER()
// * MI_DOCK
MI_DOCK::MI_DOCK(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_BASE(toolhead, _("Dock Position"), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {}

void MI_DOCK::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenToolheadDetailDock>(toolhead()));
}

// * MI_PICK_PARK
MI_PICK_PARK::MI_PICK_PARK(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_BASE(toolhead, string_view_utf8()) {
    update();
}

void MI_PICK_PARK::update() {
    const auto picked_tool = PhysicalToolIndex::currently_selected();
    is_picked = match(
        toolhead(), //
        [&](AllTools) { return std::holds_alternative<PhysicalToolIndex>(picked_tool); }, //
        [&](PhysicalToolIndex tool) { return stdext::holds_value(picked_tool, tool); } //
    );

    // Do not show "Pick Tool" at all for all_toolheads, only "Park Tool" that gets disabled when no tool is selected
    SetLabel(_(is_picked || (toolhead() == all_toolheads) ? N_("Park Tool") : N_("Pick Tool")));

    // If we're in all toolheads mode, allow only unpicking the tool
    set_enabled((toolhead() != all_toolheads) || is_picked);
}

void MI_PICK_PARK::click(IWindowMenu &) {
    marlin_client::gcode("G27 P0 Z5"); // Lift Z if not high enough
    marlin_client::gcode_printf("T%d S1 L0 D0", (!is_picked && toolhead() != all_toolheads) ? std::get<PhysicalToolIndex>(toolhead()).to_raw() : PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
    window_dlg_wait_t::wait_for_gcodes_to_finish();
    update();
}

#endif

// * MI_FILAMENT_SENSORS
MI_FILAMENT_SENSORS::MI_FILAMENT_SENSORS(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_BASE(toolhead, _("Filament Sensors Tuning"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {}

void MI_FILAMENT_SENSORS::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenToolheadDetailFS>(toolhead()));
}

#if HAS_SELFTEST() && FILAMENT_SENSOR_IS_ADC()
// * MI_CALIBRATE_FILAMENT_SENSORS
MI_CALIBRATE_FILAMENT_SENSORS::MI_CALIBRATE_FILAMENT_SENSORS(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_BASE(toolhead, string_view_utf8()) {
    update();
}

void MI_CALIBRATE_FILAMENT_SENSORS::update() {
    SetLabel((HAS_SIDE_FSENSOR() || toolhead() == all_toolheads) ? _("Calibrate Filament Sensors") : _("Calibrate Filament Sensor"));
}

void MI_CALIBRATE_FILAMENT_SENSORS::click(IWindowMenu &) {
    if (MsgBoxQuestion(_("Perform filament sensors calibration? This discards previous filament sensors calibration."), Responses_YesNo) == Response::No) {
        return;
    }

    if (toolhead() == all_toolheads) {
        marlin_client::gcode_printf("M1981 F%i", (1 << HOTENDS) - 1);
    } else {
        marlin_client::gcode_printf("M1981 T%i", std::get<PhysicalToolIndex>(toolhead()).to_raw());
    }
}
#endif

// * MI_NOZZLE_OFFSET
MI_NOZZLE_OFFSET::MI_NOZZLE_OFFSET(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_BASE(toolhead, _("Nozzle Offset"), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {}

void MI_NOZZLE_OFFSET::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenToolheadDetailNozzleOffset>(toolhead()));
}

// * ScreenToolheadDetail
ScreenToolheadDetail::ScreenToolheadDetail(Toolhead toolhead)
    : ScreenMenu({})
    , toolhead(toolhead) //
{

#if HAS_TOOLCHANGER()
    if (toolhead == all_toolheads) {
        header.SetText(_("ALL TOOLS"));
    } else if (prusa_toolchanger.is_toolchanger_enabled()) {
        header.SetText(_("TOOL %d").formatted(title_params, std::get<PhysicalToolIndex>(toolhead).display_index()));
    } else {
        header.SetText(_("TOOLHEAD"));
    }
#else
    header.SetText(_("PRINTHEAD"));
#endif

    menu_set_toolhead(container, toolhead);

    // Do not show certain items until printer setup is done
    if (!config_store().printer_hw_config_done.get()) {
#if HAS_TOOLCHANGER()
        container.Item<MI_DOCK>().set_is_hidden();
        container.Item<MI_NOZZLE_OFFSET>().set_is_hidden();
        container.Item<MI_PICK_PARK>().set_is_hidden();
#endif
#if HAS_SELFTEST() && FILAMENT_SENSOR_IS_ADC()
        container.Item<MI_CALIBRATE_FILAMENT_SENSORS>().set_is_hidden();
#endif
    }

    // Some options don't make sense for AllToolheads
    if (toolhead == all_toolheads) {
#if HAS_TOOLCHANGER()
        container.Item<MI_DOCK>().set_is_hidden();
        container.Item<MI_NOZZLE_OFFSET>().set_is_hidden();
#endif
    }

    // On XL, the first toolhead is the reference with XYZ offsets of 0,
    // so it doesn't make sense to show the nozzle offset menu for it.
    // On INDX printers with a tool offset sensor, there is no reference
    // toolhead, so all toolheads should show the nozzle offset settings.
    if (toolhead == default_toolhead) {
#if HAS_TOOLCHANGER() && !HAS_TOOL_OFFSET_SENSOR()
        // Nozzle offset is always relative to the first tool, so it does not make sense to calibrate it for tool 0 on XL
        container.Item<MI_NOZZLE_OFFSET>().set_is_hidden();
#endif
    }

#if HAS_TOOLCHANGER()
    // Some options don't make sense if a toolchanger is disabled (single-tool XL)
    if (!prusa_toolchanger.is_toolchanger_enabled()) {
        container.Item<MI_PICK_PARK>().set_is_hidden();
        container.Item<MI_DOCK>().set_is_hidden();
    }
#endif

#if HAS_HT_HOTEND()
    // The HT hotend has no silicone sock and cannot take one (its heat emission is set by the
    // nickel-plated surface), so the sock toggle is meaningless for it — high_temp is an
    // auto-detected HotendType that never appears in the user-selectable list.
    static_assert(PhysicalToolIndex::count == 1);
    if (config_store().hotend_type.get(0) == HotendType::high_temp) {
        container.Item<MI_HOTEND_SOCK_OR_TYPE>().set_is_hidden();
    }
#endif
}

// * MI_TOOLHEAD
MI_TOOLHEAD::MI_TOOLHEAD(Toolhead toolhead)
    : IWindowMenuItem({}, nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes)
    , toolhead(toolhead) //
{
    if (toolhead == all_toolheads) {
        SetLabel(_("All Tools"));

    } else {
        const PhysicalToolIndex ix = std::get<PhysicalToolIndex>(toolhead);
        SetLabel(ix.display_name(label_params));
        set_is_hidden(!ix.is_enabled());
    }
}

void MI_TOOLHEAD::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenToolheadDetail>(toolhead));
}

#if HAS_TOOLCHANGER()

// * ScreenToolheadSettingsList
ScreenToolheadSettingsList::ScreenToolheadSettingsList()
    : ScreenMenu(_("TOOLS SETTINGS")) //
{}

#endif
