#include "MItem_print.hpp"
#include "marlin_client.hpp"
#include "common/conversions.hpp"
#include "menu_vars.h"
#include "WindowMenuSpin.hpp"
#include "img_resources.hpp"
#include <numeric_input_config_common.hpp>
#include <option/has_indx.h>
#include <utils/string_builder.hpp>
#if HAS_INDX()
    #include <fanctl.hpp>
#endif

/*****************************************************************************/

namespace {
const img::Resource *icon_for_tool(const std::variant<PhysicalToolIndex, CurrentlySelectedTool> &tool) {
    return match(
        tool,
        [](PhysicalToolIndex t) -> const img::Resource * {
            return stdext::holds_value(PhysicalToolIndex::currently_selected(), t)
                ? &img::arrow_right_10x16
                : nullptr;
        },
        // Top-level CurrentlySelectedTool item (when not in per-tool mode) keeps the nozzle icon.
        [](CurrentlySelectedTool) -> const img::Resource * { return &img::nozzle_16x16; });
}
} // namespace

// MI_NOZZLE_TARGET_TEMP
MI_NOZZLE_TARGET_TEMP::MI_NOZZLE_TARGET_TEMP(std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool)
    : NumericInputConfigHolder { numeric_input_config::nozzle_temperature(resolve_tool_index(tool).value_or(PhysicalToolIndex::from_raw(0))) }
    , WiSpin(0, NumericInputConfigHolder::owned_config, string_view_utf8 {}, icon_for_tool(tool))
    , tool_(tool) {
    SetLabel(match(
        tool_,
        [&](PhysicalToolIndex t) { return t.display_name(label_params_); },
        [](CurrentlySelectedTool) {
#if HAS_MINI_DISPLAY()
            return _("Nozzle");
#else
            return _("Nozzle Temperature");
#endif
        }));
}

void MI_NOZZLE_TARGET_TEMP::OnClick() {
    if (resolved_tool_.has_value()) {
        marlin_client::set_target_nozzle(static_cast<int16_t>(value()), *resolved_tool_);
    }
}

void MI_NOZZLE_TARGET_TEMP::Loop() {
    WiSpin::Loop();

    if (is_edited()) {
        // Don't change the item under user's hands
        return;
    }

    // Resolve the tool outside of editing, so that a tool change mid-edit doesn't cause writing to a wrong tool
    resolved_tool_ = resolve_enabled_tool_index(tool_);

    set_enabled(resolved_tool_.has_value());

    if (resolved_tool_.has_value()) {
        set_value(marlin_vars().hotend(*resolved_tool_).target_nozzle);
    } else {
        set_value(config().special_value);
    }
}

/*****************************************************************************/
// MI_INFO_NOZZLE_TEMP
MI_INFO_NOZZLE_TEMP::MI_INFO_NOZZLE_TEMP(std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool)
    : MenuItemAutoUpdatingLabel(string_view_utf8 {}, standard_print_format::temp_c,
        [](auto *item) { return static_cast<MI_INFO_NOZZLE_TEMP *>(item)->value(); })
    , tool_(tool) {
    SetLabel(match(
        tool_,
        [&](PhysicalToolIndex t) { return t.display_name(label_params_); },
        [](CurrentlySelectedTool) { return _("Nozzle Temp"); }));
}

float MI_INFO_NOZZLE_TEMP::value() const {
    const auto tool = resolve_enabled_tool_index(tool_);
    if (!tool.has_value()) {
        return NAN;
    }

    return marlin_vars().hotend(*tool).temp_nozzle;
}

/*****************************************************************************/
// MI_INFO_HEATBREAK_TEMP
#if HAS_TEMP_HEATBREAK
MI_INFO_HEATBREAK_TEMP::MI_INFO_HEATBREAK_TEMP(std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool)
    : MenuItemAutoUpdatingLabel(string_view_utf8 {}, standard_print_format::temp_c,
        [](auto *item) { return static_cast<MI_INFO_HEATBREAK_TEMP *>(item)->value(); })
    , tool_(tool) {
    SetLabel(match(
        tool_,
        [&](PhysicalToolIndex t) { return t.display_name(label_params_); },
        [](CurrentlySelectedTool) { return _("Heatbreak Temp"); }));
}

float MI_INFO_HEATBREAK_TEMP::value() const {
    const auto tool = resolve_enabled_tool_index(tool_);
    if (!tool.has_value()) {
        return NAN;
    }

    return marlin_vars().hotend(*tool).temp_heatbreak;
}
#endif

/*****************************************************************************/
// MI_HEATBED

MI_HEATBED::MI_HEATBED()
    : WiSpin(uint8_t(marlin_vars().target_bed), numeric_input_config::bed_temperature, _(label), &img::heatbed_16x16, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_HEATBED::OnClick() {
    marlin_client::set_target_bed(static_cast<int16_t>(value()));
}

/*****************************************************************************/
// MI_PRINTFAN

static constexpr NumericInputConfig printfan_spin_config = {
    .max_value = 100,
    .special_value = 0,
    .unit = Unit::percent,
};

MI_PRINTFAN::MI_PRINTFAN()
    : WiSpin(val_mapping(false, marlin_vars().print_fan_speed, 255, 100),
        printfan_spin_config, _(label), &img::fan_16x16, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_PRINTFAN::OnClick() {
    marlin_client::set_fan_speed(val_mapping(true, static_cast<uint8_t>(value()), 100, 255));
}

/*****************************************************************************/
// MI_DOCKFAN
#if HAS_INDX()
MI_DOCKFAN::MI_DOCKFAN()
    : WiSpin(val_mapping(false, Fans::dock_fan().get_pwm(), 255, 100),
        printfan_spin_config, _(label), &img::fan_16x16, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_DOCKFAN::OnClick() {
    // Route through marlin_server (gcode queue) instead of touching the fan
    // controller directly — set_pwm is called from the same thread as M106 P6
    // and never races with the 1 kHz Fans::tick() ISR.
    marlin_client::gcode_printf("M106 P6 S%u",
        val_mapping(true, static_cast<uint8_t>(value()), 100, 255));
}
#endif

/*****************************************************************************/
// MI_SPEED

static constexpr NumericInputConfig print_speed_spin_config = {
    .min_value = 10,
    .max_value = 300,
    .unit = Unit::percent,
};

MI_SPEED::MI_SPEED()
    : WiSpin(uint16_t(marlin_vars().print_speed), print_speed_spin_config, _(label), &img::speed_16x16, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SPEED::OnClick() {
    marlin_client::set_print_speed(static_cast<uint16_t>(value()));
}

/*****************************************************************************/
// MI_FLOW_FACTOR
static constexpr NumericInputConfig flowfact_spin_config {
    .min_value = 50,
    .max_value = 150,
    .special_value = 0,
    .special_value_str = N_("N/A"),
    .unit = Unit::percent,
};

MI_FLOW_FACTOR::MI_FLOW_FACTOR(std::variant<VirtualToolIndex, CurrentlySelectedTool> tool)
    : WiSpin(0, flowfact_spin_config, string_view_utf8 {}, nullptr)
    , tool_(tool) {
    SetLabel(match(
        tool_,
        [&](VirtualToolIndex t) { return t.display_name(label_params_); },
        [](CurrentlySelectedTool) { return _("Flow Factor"); }));
}

void MI_FLOW_FACTOR::OnClick() {
    if (resolved_tool_.has_value()) {
        marlin_client::set_flow_factor(static_cast<uint16_t>(value()), *resolved_tool_);
    }
}

void MI_FLOW_FACTOR::Loop() {
    WiSpin::Loop();

    if (is_edited()) {
        // Don't change the item under user's hands
        return;
    }

    // Resolve the tool outside of editing, so that a tool change mid-edit doesn't cause writing to a wrong tool
    resolved_tool_ = resolve_enabled_tool_index(tool_);

    set_enabled(resolved_tool_.has_value());

    if (resolved_tool_.has_value()) {
        set_value(marlin_vars().virtual_tools[*resolved_tool_].flow_factor);
    } else {
        set_value(config().special_value);
    }
}
