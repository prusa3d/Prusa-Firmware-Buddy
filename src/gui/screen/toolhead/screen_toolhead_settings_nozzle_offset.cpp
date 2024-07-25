#include "screen_toolhead_settings_nozzle_offset.hpp"

#include <str_utils.hpp>

using namespace screen_toolhead_settings;

static constexpr std::array<NumericInputConfig, 3> offset_configs {
    NumericInputConfig {
        .min_value = X_MIN_OFFSET,
        .max_value = X_MAX_OFFSET,
        .step = 0.01,
        .max_decimal_places = 2,
        .unit = Unit::millimeter,
    },
    NumericInputConfig {
        .min_value = Y_MIN_OFFSET,
        .max_value = Y_MAX_OFFSET,
        .step = 0.01,
        .max_decimal_places = 2,
        .unit = Unit::millimeter,
    },
    NumericInputConfig {
        .min_value = Z_MIN_OFFSET,
        .max_value = Z_MAX_OFFSET,
        .step = 0.01,
        .max_decimal_places = 2,
        .unit = Unit::millimeter,
    },
};

// * MI_NOZZLE_OFFSET_COMPONENT
MI_NOZZLE_OFFSET_COMPONENT::MI_NOZZLE_OFFSET_COMPONENT(uint8_t component, Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_SPIN(toolhead, 0, offset_configs[component], string_view_utf8())
    , component_(component) //
{
    SetLabel(_("Offset %c").formatted(label_params_, "XYZ"[component]));
}

float MI_NOZZLE_OFFSET_COMPONENT::read_value_impl(ToolheadIndex ix) {
    return hotend_offset[ix].pos[component_];
}

void MI_NOZZLE_OFFSET_COMPONENT::store_value_impl(ToolheadIndex ix, float set) {
    hotend_offset[ix].pos[component_] = set;
    prusa_toolchanger.save_tool_offsets();
}

// * ScreenToolheadDetailNozzleOffset
ScreenToolheadDetailNozzleOffset::ScreenToolheadDetailNozzleOffset(Toolhead toolhead)
    : ScreenMenu(_("NOZZLE_OFFSET"))
    , toolhead(toolhead) //
{
    menu_set_toolhead(container, toolhead);
}
