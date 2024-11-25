#include "screen_toolhead_settings_fs.hpp"

using namespace screen_toolhead_settings;

static constexpr NumericInputConfig fs_span_spin_config = {
#if PRINTER_IS_PRUSA_XL()
    .min_value = 50,
    .max_value = 1500,
    .step = 10,
#else
    .min_value = 50000,
    .max_value = 2500000,
    .step = 1000,
#endif
    .special_value = -1,
    .special_value_str = N_("-"),
};

static constexpr NumericInputConfig fs_ref_spin_config = {
    .max_value = 99999,
    .special_value = -1,
    .special_value_str = N_("-"),
};

// * MI_FS_SPAN
MI_FS_SPAN::MI_FS_SPAN(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_SPIN(toolhead, 0, fs_span_spin_config, string_view_utf8::MakeCPUFLASH("FS Span")) {
    update();
}

float MI_FS_SPAN::read_value_impl(ToolheadIndex ix) {
    return config_store().get_extruder_fs_value_span(ix);
}

void MI_FS_SPAN::store_value_impl(ToolheadIndex ix, float set) {
    config_store().set_extruder_fs_value_span(ix, set);
}

// * MI_FS_REF
MI_FS_REF::MI_FS_REF(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_SPIN(toolhead, 0, fs_ref_spin_config, string_view_utf8::MakeCPUFLASH("FS NINS Ref")) {
    update();
}

float MI_FS_REF::read_value_impl(ToolheadIndex ix) {
    return config_store().get_extruder_fs_ref_nins_value(ix);
}

void MI_FS_REF::store_value_impl(ToolheadIndex ix, float set) {
    config_store().set_extruder_fs_ref_nins_value(ix, set);
}

#if HAS_ADC_SIDE_FSENSOR()
// * MI_SIDE_FS_SPAN
MI_SIDE_FS_SPAN::MI_SIDE_FS_SPAN(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_SPIN(toolhead, 0, fs_span_spin_config, string_view_utf8::MakeCPUFLASH("Side FS Span")) {
    update();
}

float MI_SIDE_FS_SPAN::read_value_impl(ToolheadIndex ix) {
    return config_store().get_side_fs_value_span(ix);
}

void MI_SIDE_FS_SPAN::store_value_impl(ToolheadIndex ix, float set) {
    config_store().set_side_fs_value_span(ix, set);
}

// * MI_SIDE_FS_REF
MI_SIDE_FS_REF::MI_SIDE_FS_REF(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_SPIN(toolhead, 0, fs_ref_spin_config, string_view_utf8::MakeCPUFLASH("Side FS NINS Ref")) {
    update();
}

float MI_SIDE_FS_REF::read_value_impl(ToolheadIndex ix) {
    return config_store().get_side_fs_ref_nins_value(ix);
}

void MI_SIDE_FS_REF::store_value_impl(ToolheadIndex ix, float set) {
    config_store().set_side_fs_ref_nins_value(ix, set);
}
#endif

// * ScreenToolheadDetailFS
ScreenToolheadDetailFS::ScreenToolheadDetailFS(Toolhead toolhead)
    : ScreenMenu(_("FILAMENT SENSORS"))
    , toolhead(toolhead) //
{
    menu_set_toolhead(container, toolhead);
}
