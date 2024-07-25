#include "screen_toolhead_settings.hpp"

#include <common/nozzle_diameter.hpp>
#include <ScreenHandler.hpp>
#include <img_resources.hpp>

#include "screen_toolhead_settings_fs.hpp"
#include "screen_toolhead_settings_dock.hpp"
#include "screen_toolhead_settings_nozzle_offset.hpp"

using namespace screen_toolhead_settings;

static constexpr NumericInputConfig nozzle_diameter_spin_config_with_special = [] {
    NumericInputConfig result = nozzle_diameter_spin_config;
    result.special_value = 0;
    result.special_value_str = N_("-");
    return result;
}();

// * MI_NOZZLE_DIAMETER
MI_NOZZLE_DIAMETER::MI_NOZZLE_DIAMETER(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_SPIN(toolhead, 0, nozzle_diameter_spin_config_with_special, _("Nozzle Diameter")) {
    update();
}

float MI_NOZZLE_DIAMETER::read_value_impl(ToolheadIndex ix) {
    return config_store().get_nozzle_diameter(ix);
}

void MI_NOZZLE_DIAMETER::store_value_impl(ToolheadIndex ix, float set) {
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
MI_HOTEND_TYPE::MI_HOTEND_TYPE()
    : WiStoreEnumSwitch(_("Hotend Type"), hotend_type_names, true, hotend_type_supported) {}

// * MI_NOZZLE_SOCK
MI_NOZZLE_SOCK::MI_NOZZLE_SOCK()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().hotend_type.get() == HotendType::stock_with_sock, _("Nextruder Silicone Sock"), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_NOZZLE_SOCK::OnChange([[maybe_unused]] size_t old_index) {
    config_store().hotend_type.set(index ? HotendType::stock_with_sock : HotendType::stock);
}
#endif

// * MI_NOZZLE_HARDENED
MI_NOZZLE_HARDENED::MI_NOZZLE_HARDENED(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_TOGGLE(toolhead, false, _("Nozzle Hardened")) //
{
    update();
}

bool MI_NOZZLE_HARDENED::read_value_impl(ToolheadIndex ix) {
    return config_store().nozzle_is_hardened.get().test(ix);
}

void MI_NOZZLE_HARDENED::store_value_impl(ToolheadIndex ix, bool set) {
    config_store().nozzle_is_hardened.apply([&](auto &item) {
        item.set(ix, set);
    });
}

// * MI_NOZZLE_HIGH_FLOW
MI_NOZZLE_HIGH_FLOW::MI_NOZZLE_HIGH_FLOW(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_TOGGLE(toolhead, false, _("Nozzle High-flow")) //
{
    update();
}

bool MI_NOZZLE_HIGH_FLOW::read_value_impl(ToolheadIndex ix) {
    return config_store().nozzle_is_high_flow.get().test(ix);
}

void MI_NOZZLE_HIGH_FLOW::store_value_impl(ToolheadIndex ix, bool set) {
    config_store().nozzle_is_high_flow.apply([&](auto &item) {
        item.set(ix, set);
    });
}

#if HAS_TOOLCHANGER()
// * MI_DOCK
MI_DOCK::MI_DOCK(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC(toolhead, _("Dock Position"), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {}

void MI_DOCK::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenToolheadDetailDock>(toolhead()));
}

// * MI_PICK_PARK
MI_PICK_PARK::MI_PICK_PARK(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC(toolhead, false, _("Pick Tool")) {
    update();
}

void MI_PICK_PARK::update(bool update_value) {
    if (update_value) {
        const auto picked_tool = prusa_toolchanger.detect_tool_nr();
        set_value((toolhead() == all_toolheads) ? (picked_tool != PrusaToolChanger::MARLIN_NO_TOOL_PICKED) : (picked_tool == std::get<ToolheadIndex>(toolhead())), false);
    }

    // If we're in all toolheads mode, allow only unpicking the tool
    set_is_enabled((toolhead() != all_toolheads) || value());
}

void MI_PICK_PARK::OnChange(size_t) {
    marlin_client::gcode("G27 P0 Z5"); // Lift Z if not high enough
    marlin_client::gcode_printf("T%d S1 L0 D0", (value() && toolhead() != all_toolheads) ? std::get<ToolheadIndex>(toolhead()) : PrusaToolChanger::MARLIN_NO_TOOL_PICKED);

    update(false);
}

#endif

// * MI_FILAMENT_SENSORS
MI_FILAMENT_SENSORS::MI_FILAMENT_SENSORS(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC(toolhead, _("Filament Sensors Tuning"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {}

void MI_FILAMENT_SENSORS::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenToolheadDetailFS>(toolhead()));
}

#if FILAMENT_SENSOR_IS_ADC()
// * MI_CALIBRATE_FILAMENT_SENSORS
MI_CALIBRATE_FILAMENT_SENSORS::MI_CALIBRATE_FILAMENT_SENSORS(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC(toolhead, string_view_utf8()) {
    update();
}

void MI_CALIBRATE_FILAMENT_SENSORS::update() {
    SetLabel((HAS_SIDE_FSENSOR() || toolhead() == all_toolheads) ? _("Calibrate Filament Sensors") : _("Calibrate Filament Sensor"));
}

void MI_CALIBRATE_FILAMENT_SENSORS::click(IWindowMenu &) {
    if (MsgBoxQuestion(_("Perform filament sensors calibration? This discards previous filament sensors calibration."), Responses_YesNo) == Response::No) {
        return;
    }

    marlin_client::test_start_with_data(stmFSensor, (toolhead() == all_toolheads) ? ToolMask::AllTools : static_cast<ToolMask>(1 << std::get<ToolheadIndex>(toolhead())));
}
#endif

// * MI_NOZZLE_OFFSET
MI_NOZZLE_OFFSET::MI_NOZZLE_OFFSET(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC(toolhead, _("Nozzle Offset"), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {}

void MI_NOZZLE_OFFSET::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenToolheadDetailNozzleOffset>(toolhead()));
}

// * ScreenToolheadDetail
ScreenToolheadDetail::ScreenToolheadDetail(Toolhead toolhead)
    : ScreenMenu({})
    , toolhead(toolhead) //
{
    if (toolhead == all_toolheads) {
        header.SetText(_("ALL TOOLS"));
    } else {
        header.SetText(_("TOOL %d").formatted(title_params, std::get<ToolheadIndex>(toolhead) + 1));
    }

    menu_set_toolhead(container, toolhead);

    // Do not show certain items until printer setup is done
    if (!config_store().printer_setup_done.get()) {
#if HAS_TOOLCHANGER()
        container.Item<MI_DOCK>().set_is_hidden();
        container.Item<MI_NOZZLE_OFFSET>().set_is_hidden();
        container.Item<MI_PICK_PARK>().set_is_hidden();
#endif
#if FILAMENT_SENSOR_IS_ADC()
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

    if (toolhead == default_toolhead) {
#if HAS_TOOLCHANGER()
        // Nozzle offset is always relative to the first tool, so it does not make sense to calibrate it for tool 0
        container.Item<MI_NOZZLE_OFFSET>().set_is_hidden();
#endif
    }
}

// * MI_TOOLHEAD
MI_TOOLHEAD::MI_TOOLHEAD(Toolhead toolhead)
    : IWindowMenuItem({}, nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes)
    , toolhead(toolhead) //
{
    if (toolhead == all_toolheads) {
        SetLabel(_("All Tools"));

    } else {
        const ToolheadIndex ix = std::get<ToolheadIndex>(toolhead);
        SetLabel(_("Tool %d").formatted(label_params, ix + 1));
#if HAS_TOOLCHANGER()
        set_is_hidden(!prusa_toolchanger.is_tool_enabled(ix));
#endif
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
