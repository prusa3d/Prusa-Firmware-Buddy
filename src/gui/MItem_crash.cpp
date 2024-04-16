#include "config_features.h"
// TODO do it in cmake
#if ENABLED(CRASH_RECOVERY)

    #include "MItem_crash.hpp"
    #include "menu_spin_config.hpp"
    #include "../lib/Marlin/Marlin/src/module/stepper/trinamic.h"
    #include "../Marlin/src/feature/prusa/crash_recovery.hpp"
    #include "../Marlin/src/feature/phase_stepping/phase_stepping.hpp"
    #include <config_store/store_instance.hpp>

MI_CRASH_DETECTION::MI_CRASH_DETECTION()
    : WI_ICON_SWITCH_OFF_ON_t(crash_s.is_enabled(), _(label), nullptr, is_enabled_t::yes,
    #if (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5)
        is_hidden_t::dev
    #else
        is_hidden_t::no
    #endif // (( PRINTER_IS_PRUSA_MK4) || ( PRINTER_IS_PRUSA_MK3_5))
    ) {
}

void MI_CRASH_DETECTION::Loop() {
    #if HAS_PHASE_STEPPING() && !HAS_BURST_STEPPING()
    // Enable or disable according to the current phase stepping state. We can't really use
    // invalidation to reduce calls to config_store, as Print() happens and resets the state before
    // we can trap it here. At the same time, Print is not virtual.
    bool phstep_enabled = (config_store().phase_stepping_enabled_x.get() || config_store().phase_stepping_enabled_y.get());
    if (phstep_enabled) {
        set_value(false, false);
        Disable();
    } else {
        set_value(crash_s.is_enabled(), false);
        Enable();
    }
    #endif
    return WI_ICON_SWITCH_OFF_ON_t::Loop();
}

void MI_CRASH_DETECTION::OnChange(size_t /*old_index*/) {
    crash_s.enable(value());
}

MI_CRASH_SENSITIVITY_X::MI_CRASH_SENSITIVITY_X()
    : WiSpinInt(crash_s.get_sensitivity().x, SpinCnf::crash_sensitivity,
        _(label), nullptr, is_enabled_t::yes,
    #if PRINTER_IS_PRUSA_XL
        is_hidden_t::no
    #else
        is_hidden_t::dev
    #endif
    ) {
}
void MI_CRASH_SENSITIVITY_X::OnClick() {

    xy_long_t se = crash_s.get_sensitivity();
    se.x = GetVal();
    crash_s.set_sensitivity(se);
}

MI_CRASH_SENSITIVITY_Y::MI_CRASH_SENSITIVITY_Y()
    : WiSpinInt(crash_s.get_sensitivity().y, SpinCnf::crash_sensitivity,
        _(label), nullptr, is_enabled_t::yes,
    #if PRINTER_IS_PRUSA_XL
        is_hidden_t::no
    #else
        is_hidden_t::dev
    #endif
    ) {
}
void MI_CRASH_SENSITIVITY_Y::OnClick() {

    xy_long_t se = crash_s.get_sensitivity();
    se.y = GetVal();
    crash_s.set_sensitivity(se);
}

    #if PRINTER_IS_PRUSA_XL
MI_CRASH_SENSITIVITY_XY::MI_CRASH_SENSITIVITY_XY()
    : WI_SWITCH_t<3>(get_item_id_from_sensitivity(crash_s.get_sensitivity().x), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no,
        _(ITEMS[0].name), _(ITEMS[1].name), _(ITEMS[2].name)) {}

constexpr size_t MI_CRASH_SENSITIVITY_XY::get_item_id_from_sensitivity(int32_t sensitivity) {
    for (size_t i = 0; i <= std::size(ITEMS); i++) {
        if (ITEMS[i].value == sensitivity) {
            return i;
        }
    }
    return 0;
}

void MI_CRASH_SENSITIVITY_XY::OnChange([[maybe_unused]] size_t old_index) {
    int32_t sensitivity = ITEMS[index].value;
    xy_long_t se = crash_s.get_sensitivity();
    se.x = sensitivity;
    se.y = sensitivity;
    crash_s.set_sensitivity(se);
}
    #else
MI_CRASH_SENSITIVITY_XY::MI_CRASH_SENSITIVITY_XY()
    : WiSpinInt(crash_s.get_sensitivity().x, SpinCnf::crash_sensitivity,
        _(label), nullptr, is_enabled_t::yes,
        is_hidden_t::dev) {
}
void MI_CRASH_SENSITIVITY_XY::OnClick() {

    xy_long_t se = crash_s.get_sensitivity();
    se.x = GetVal();
    se.y = GetVal();
    crash_s.set_sensitivity(se);
}
    #endif

MI_CRASH_MAX_PERIOD_X::MI_CRASH_MAX_PERIOD_X()
    : WI_SPIN_CRASH_PERIOD_t(crash_s.get_max_period().x, SpinCnf::crash_max_period, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}
void MI_CRASH_MAX_PERIOD_X::OnClick() {
    xy_long_t mp = crash_s.get_max_period();
    mp.x = GetVal();
    crash_s.set_max_period(mp);
}

MI_CRASH_MAX_PERIOD_Y::MI_CRASH_MAX_PERIOD_Y()
    : WI_SPIN_CRASH_PERIOD_t(crash_s.get_max_period().y, SpinCnf::crash_max_period, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}
void MI_CRASH_MAX_PERIOD_Y::OnClick() {
    xy_long_t mp = crash_s.get_max_period();
    mp.y = GetVal();
    crash_s.set_max_period(mp);
}

#endif // ENABLED(CRASH_RECOVERY)

#if ANY(CRASH_RECOVERY, POWER_PANIC)

MI_POWER_PANICS::MI_POWER_PANICS()
    : WI_INFO_t(config_store().power_panics_count.get(), _(label)) {}

MI_CRASHES_X_LAST::MI_CRASHES_X_LAST()
    : WI_INFO_t(crash_s.counters.get(Crash_s::Counter::axis_crash_x), _(label),
    #if PRINTER_IS_PRUSA_XL
        is_hidden_t::no
    #else
        is_hidden_t::dev
    #endif
    ) {
}

MI_CRASHES_Y_LAST::MI_CRASHES_Y_LAST()
    : WI_INFO_t(crash_s.counters.get(Crash_s::Counter::axis_crash_y), _(label),
    #if PRINTER_IS_PRUSA_XL
        is_hidden_t::no
    #else
        is_hidden_t::dev
    #endif
    ) {
}

MI_CRASHES_X::MI_CRASHES_X()
    : WI_INFO_t(config_store().crash_count_x.get(), _(label),
    #if PRINTER_IS_PRUSA_XL
        is_hidden_t::no
    #else
        is_hidden_t::dev
    #endif
    ) {
}

MI_CRASHES_Y::MI_CRASHES_Y()
    : WI_INFO_t(config_store().crash_count_y.get(), _(label),
    #if PRINTER_IS_PRUSA_XL
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
#endif // ANY(CRASH_RECOVERY, POWER_PANIC)
