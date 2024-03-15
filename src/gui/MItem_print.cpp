#include "MItem_print.hpp"
#include "marlin_client.hpp"
#include "common/conversions.hpp"
#include "menu_vars.h"
#include "WindowMenuSpin.hpp"
#include "img_resources.hpp"
#if ENABLED(PRUSA_TOOLCHANGER)
    #include "module/prusa/toolchanger.h"
#endif
#if HAS_MMU2()
    #include <feature/prusa/MMU2/mmu2_mk4.h>
#endif

/*****************************************************************************/
// MI_NOZZLE_ABSTRACT
is_hidden_t MI_NOZZLE_ABSTRACT::is_hidden([[maybe_unused]] uint8_t tool_nr) {
#if ENABLED(PRUSA_TOOLCHANGER)
    return prusa_toolchanger.is_tool_enabled(tool_nr) ? is_hidden_t::no : is_hidden_t::yes;
#else
    return is_hidden_t::no;
#endif
}

static constexpr spin_off_opt_t spin_off_option = PRINTER_IS_PRUSA_MINI ? spin_off_opt_t::no : spin_off_opt_t::yes;

static const SpinConfig<int> nozzle_spin_config = SpinConfig<int>(MenuVars::GetNozzleRange(), SpinUnit::celsius, spin_off_option);
static const SpinConfig<int> bed_spin_config = SpinConfig<int>(MenuVars::GetBedRange(), SpinUnit::celsius, spin_off_option);

MI_NOZZLE_ABSTRACT::MI_NOZZLE_ABSTRACT(uint8_t tool_nr, [[maybe_unused]] const char *label)
    : WiSpinInt(uint16_t(marlin_vars()->hotend(tool_nr).target_nozzle), nozzle_spin_config,
#if ENABLED(PRUSA_TOOLCHANGER)
        prusa_toolchanger.is_toolchanger_enabled() ? _(label) : _(generic_label),
#else
        _(generic_label),
#endif
        &img::nozzle_16x16, is_enabled_t::yes, is_hidden(tool_nr))
    , tool_nr(tool_nr) {
}

void MI_NOZZLE_ABSTRACT::OnClick() {
    marlin_client::set_target_nozzle(GetVal(), tool_nr);
    marlin_client::set_display_nozzle(GetVal(), tool_nr);
}

/*****************************************************************************/
// MI_HEATBED
MI_HEATBED::MI_HEATBED()
    : WiSpinInt(uint8_t(marlin_vars()->target_bed),
        bed_spin_config, _(label), &img::heatbed_16x16, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_HEATBED::OnClick() {
    marlin_client::set_target_bed(GetVal());
}

static constexpr SpinConfig<int> printfan_spin_config = { { 0, 100, 1 }, SpinUnit::percent, spin_off_opt_t::yes };

/*****************************************************************************/
// MI_PRINTFAN
MI_PRINTFAN::MI_PRINTFAN()
    : WiSpinInt(val_mapping(false, marlin_vars()->print_fan_speed, 255, 100),
        printfan_spin_config, _(label), &img::fan_16x16, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_PRINTFAN::OnClick() {
    marlin_client::set_fan_speed(val_mapping(true, GetVal(), 100, 255));
}

static const SpinConfig<int> print_speed_spin_config = { MenuVars::feedrate_range, SpinUnit::percent };

/*****************************************************************************/
// MI_SPEED
MI_SPEED::MI_SPEED()
    : WiSpinInt(uint16_t(marlin_vars()->print_speed),
        print_speed_spin_config, _(label), &img::speed_16x16, is_enabled_t::yes, is_hidden_t::no) {}
void MI_SPEED::OnClick() {
    marlin_client::set_print_speed(GetVal());
}

/*****************************************************************************/
// MI_FLOWFACT_ABSTRACT
is_hidden_t MI_FLOWFACT_ABSTRACT::is_hidden([[maybe_unused]] uint8_t tool_nr) {
#if HAS_TOOLCHANGER()
    return prusa_toolchanger.is_tool_enabled(tool_nr) ? is_hidden_t::no : is_hidden_t::yes;
#elif HAS_MMU2()
    return (tool_nr == 0 || MMU2::mmu2.Enabled()) ? is_hidden_t::no : is_hidden_t::yes;
#else
    return is_hidden_t::no;
#endif
}

static constexpr const SpinConfig<int> flowfact_spin_config { { 50, 150, 1 }, SpinUnit::percent };

MI_FLOWFACT_ABSTRACT::MI_FLOWFACT_ABSTRACT(uint8_t tool_nr, [[maybe_unused]] const char *label)
    : WiSpinInt(uint16_t(marlin_vars()->hotend(tool_nr).flow_factor), flowfact_spin_config,
#if HAS_TOOLCHANGER()
        prusa_toolchanger.is_toolchanger_enabled() ? _(label) : _(generic_label),
#elif HAS_MMU2()
        MMU2::mmu2.Enabled() ? _(label) : _(generic_label),
#else
        _(generic_label),
#endif /*TOOLCHANGER or MMU2*/
        nullptr, is_enabled_t::yes, is_hidden(tool_nr))
    , tool_nr(tool_nr) {
}

void MI_FLOWFACT_ABSTRACT::OnClick() {
    marlin_client::set_flow_factor(GetVal(), tool_nr);
}
