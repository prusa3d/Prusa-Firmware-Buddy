/// @file
#include <screen_menu_advanced_footer_settings.hpp>

#include "footer_def.hpp"

/// dev item intentionally not translated
static constexpr const char *temp_align_values[] = {
    "Static",
    "Static-left",
    "Dynamic",
};

MI_LEFT_ALIGN_TEMP::MI_LEFT_ALIGN_TEMP()
    : MenuItemSwitch {
        /// dev item intentionally not translated
        string_view_utf8::MakeCPUFLASH("Temp. style"),
        temp_align_values,
        size_t(FooterItemHeater::GetDrawType()),
    } {
    showDevOnly();
}

bool MI_LEFT_ALIGN_TEMP::on_item_selected(const OnItemSelectedArgs &args) {
    FooterItemHeater::SetDrawType(footer::ItemDrawType(args.new_index));
    return true;
}

MI_SHOW_ZERO_TEMP_TARGET::MI_SHOW_ZERO_TEMP_TARGET()
    : WI_ICON_SWITCH_OFF_ON_t {
        FooterItemHeater::IsZeroTargetDrawn(),
        /// dev item intentionally not translated
        string_view_utf8::MakeCPUFLASH("Temp. show zero"),
        nullptr,
        is_enabled_t::yes,
    } {
    showDevOnly();
}

void MI_SHOW_ZERO_TEMP_TARGET::OnChange(size_t old_index) {
    old_index == 0 ? FooterItemHeater::EnableDrawZeroTarget() : FooterItemHeater::DisableDrawZeroTarget();
}

static constexpr NumericInputConfig footer_center_N_spin_config = {
    .max_value = PRINTER_IS_PRUSA_MINI() ? 3 : 5,
    .special_value = 0,
};

MI_FOOTER_CENTER_N::MI_FOOTER_CENTER_N()
    : WiSpin {
        (float)FooterLine::GetCenterN(),
        footer_center_N_spin_config,
        /// dev item intentionally not translated
        string_view_utf8::MakeCPUFLASH("Center N and Fewer Items"),
        nullptr,
        is_enabled_t::yes,
    } {
    showDevOnly();
}

void MI_FOOTER_CENTER_N::OnClick() {
    FooterLine::SetCenterN(static_cast<size_t>(value()));
}

ScreenMenuAdvancedFooterSettings::ScreenMenuAdvancedFooterSettings()
    : ScreenMenuAdvancedFooterSettingsBase {
        /// dev item intentionally not translated
        string_view_utf8::MakeCPUFLASH("FOOTER ADVANCED"),
    } {}
