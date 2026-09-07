#include "config_features.h"
#include <gui/menu_vars.h>
#include <option/has_crash_detection.h>
#include <option/has_power_panic.h>

// TODO do it in cmake
#if HAS_CRASH_DETECTION()

    #include "MItem_crash.hpp"
    #include "WindowMenuSpin.hpp"
    #include "../lib/Marlin/Marlin/src/module/stepper/trinamic.h"
    #include "../Marlin/src/feature/prusa/crash_recovery.hpp"
    #include "../Marlin/src/feature/phase_stepping/phase_stepping.hpp"
    #include <config_store/store_instance.hpp>
    #include <common/utils/algorithm_extensions.hpp>

MI_CRASH_DETECTION::MI_CRASH_DETECTION()
    : WI_ICON_SWITCH_OFF_ON_t(crash_s.is_enabled(), _(label), nullptr, is_enabled_t::yes,
    #if (PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL() || PRINTER_IS_PRUSA_XL())
        is_hidden_t::dev
    #elif (PRINTER_IS_PRUSA_XL_DEV_KIT() || PRINTER_IS_PRUSA_iX() || PRINTER_IS_PRUSA_MINI())
        is_hidden_t::no
    #else
        #error "Unknown printer"
    #endif
    ) {
}

void MI_CRASH_DETECTION::Loop() {
    #if HAS_PHASE_STEPPING() && !HAS_BURST_STEPPING()
    // Enable or disable according to the current phase stepping state. We can't really use
    // invalidation to reduce calls to config_store, as Print() happens and resets the state before
    // we can trap it here. At the same time, Print is not virtual.
    const bool phstep_enabled = config_store().get_phase_stepping_enabled();
    // If phase stepping is enabled, we can't enable crash detection and vice versa
    set_enabled(!phstep_enabled);
    set_value(!phstep_enabled && crash_s.is_enabled());
    #endif
    return WI_ICON_SWITCH_OFF_ON_t::Loop();
}

void MI_CRASH_DETECTION::OnChange(size_t /*old_index*/) {
    crash_s.enable(value());
}

static const NumericInputConfig crash_sensitivity_spin_config = {
    .min_value = static_cast<float>(MenuVars::crash_sensitivity_range.first),
    .max_value = static_cast<float>(MenuVars::crash_sensitivity_range.second),
};
static constexpr NumericInputConfig crash_max_period_spin_config = {
    .max_value = 0xFFFFF,
};

MI_CRASH_SENSITIVITY_X::MI_CRASH_SENSITIVITY_X()
    : WiSpin(crash_s.get_sensitivity().x, crash_sensitivity_spin_config, _(label), nullptr, is_enabled_t::yes, PRINTER_IS_PRUSA_XL() ? is_hidden_t::no : is_hidden_t::dev) {
}
void MI_CRASH_SENSITIVITY_X::OnClick() {

    xy_long_t se = crash_s.get_sensitivity();
    se.x = static_cast<int>(value());
    crash_s.set_sensitivity(se);
}

MI_CRASH_SENSITIVITY_Y::MI_CRASH_SENSITIVITY_Y()
    : WiSpin(crash_s.get_sensitivity().y, crash_sensitivity_spin_config, _(label), nullptr, is_enabled_t::yes, PRINTER_IS_PRUSA_XL() ? is_hidden_t::no : is_hidden_t::dev) {
}
void MI_CRASH_SENSITIVITY_Y::OnClick() {
    xy_long_t se = crash_s.get_sensitivity();
    se.y = static_cast<int>(value());
    crash_s.set_sensitivity(se);
}

MI_CRASH_SENSITIVITY_XY::MI_CRASH_SENSITIVITY_XY()
    : WiSpin(crash_s.get_sensitivity().x, crash_sensitivity_spin_config, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}
void MI_CRASH_SENSITIVITY_XY::OnClick() {
    crash_s.set_sensitivity({ static_cast<long>(value()), static_cast<long>(value()) });
}

MI_CRASH_MAX_PERIOD_X::MI_CRASH_MAX_PERIOD_X()
    : WiSpin(crash_s.get_max_period().x, crash_max_period_spin_config, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}
void MI_CRASH_MAX_PERIOD_X::OnClick() {
    xy_long_t mp = crash_s.get_max_period();
    mp.x = static_cast<int>(value());
    crash_s.set_max_period(mp);
}

MI_CRASH_MAX_PERIOD_Y::MI_CRASH_MAX_PERIOD_Y()
    : WiSpin(crash_s.get_max_period().y, crash_max_period_spin_config, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}
void MI_CRASH_MAX_PERIOD_Y::OnClick() {
    xy_long_t mp = crash_s.get_max_period();
    mp.y = static_cast<int>(value());
    crash_s.set_max_period(mp);
}

#endif

#if HAS_CRASH_DETECTION() || HAS_POWER_PANIC()

MI_POWER_PANICS::MI_POWER_PANICS()
    : WI_INFO_t(config_store().power_panics_count.get(), _(label)) {}

MI_CRASHES_X_LAST::MI_CRASHES_X_LAST()
    : WI_INFO_t(crash_s.counters.get(Crash_s::Counter::axis_crash_x), _(label),
    #if PRINTER_IS_PRUSA_XL()
        is_hidden_t::no
    #else
        is_hidden_t::dev
    #endif
    ) {
}

MI_CRASHES_Y_LAST::MI_CRASHES_Y_LAST()
    : WI_INFO_t(crash_s.counters.get(Crash_s::Counter::axis_crash_y), _(label),
    #if PRINTER_IS_PRUSA_XL()
        is_hidden_t::no
    #else
        is_hidden_t::dev
    #endif
    ) {
}

MI_CRASHES_X::MI_CRASHES_X()
    : WI_INFO_t(config_store().crash_count_x.get(), _(label),
    #if PRINTER_IS_PRUSA_XL()
        is_hidden_t::no
    #else
        is_hidden_t::dev
    #endif
    ) {
}

MI_CRASHES_Y::MI_CRASHES_Y()
    : WI_INFO_t(config_store().crash_count_y.get(), _(label),
    #if PRINTER_IS_PRUSA_XL()
        is_hidden_t::no
    #else
        is_hidden_t::dev
    #endif
    ) {
}

    #if HAS_DRIVER(TMC2130)
MI_CRASH_FILTERING::MI_CRASH_FILTERING()
    : WI_ICON_SWITCH_OFF_ON_t(crash_s.get_filter(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_CRASH_FILTERING::OnChange([[maybe_unused]] size_t old_index) {
    crash_s.set_filter(value());
}
    #endif
#endif
