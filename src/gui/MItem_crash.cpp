#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
// TODO do it in cmake
#if ENABLED(CRASH_RECOVERY)

    #include "MItem_crash.hpp"
    #include "screen_menus.hpp"
    #include "menu_spin_config.hpp"
    #include "../lib/Marlin/Marlin/src/module/stepper/trinamic.h"
    #include "../Marlin/src/feature/prusa/crash_recovery.h"

MI_CRASH_DETECTION::MI_CRASH_DETECTION()
    : WI_SWITCH_OFF_ON_t(0, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
    index = crash_s.is_enabled();
}

void MI_CRASH_DETECTION::OnChange(size_t old_index) {
    crash_s.enable(index);
}

MI_CRASH_SENSITIVITY_X::MI_CRASH_SENSITIVITY_X()
    : WiSpinInt(crash_s.get_sensitivity().x, SpinCnf::crash_sensitivity_2209, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}
void MI_CRASH_SENSITIVITY_X::OnClick() {

    xy_long_t se = crash_s.get_sensitivity();
    se.x = GetVal();
    crash_s.set_sensitivity(se);
}

MI_CRASH_SENSITIVITY_Y::MI_CRASH_SENSITIVITY_Y()
    : WiSpinInt(crash_s.get_sensitivity().y, SpinCnf::crash_sensitivity_2209, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}
void MI_CRASH_SENSITIVITY_Y::OnClick() {

    xy_long_t se = crash_s.get_sensitivity();
    se.y = GetVal();
    crash_s.set_sensitivity(se);
}

constexpr float _DASU[] = DEFAULT_AXIS_STEPS_PER_UNIT;

MI_CRASH_MAX_PERIOD_X::MI_CRASH_MAX_PERIOD_X()
    : WI_SPIN_CRASH_PERIOD_t(crash_s.get_max_period().x, SpinCnf::crash_max_period_2209, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}
void MI_CRASH_MAX_PERIOD_X::OnClick() {
    xy_long_t mp = crash_s.get_max_period();
    mp.x = GetVal();
    crash_s.set_max_period(mp);
}

MI_CRASH_MAX_PERIOD_Y::MI_CRASH_MAX_PERIOD_Y()
    : WI_SPIN_CRASH_PERIOD_t(crash_s.get_max_period().y, SpinCnf::crash_max_period_2209, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}
void MI_CRASH_MAX_PERIOD_Y::OnClick() {
    xy_long_t mp = crash_s.get_max_period();
    mp.y = GetVal();
    crash_s.set_max_period(mp);
}

#endif // ENABLED(CRASH_RECOVERY)

#if ANY(CRASH_RECOVERY, POWER_PANIC)

MI_POWER_PANICS::MI_POWER_PANICS()
    : WI_INFO_t(variant8_get_ui16(eeprom_get_var(EEVAR_POWER_COUNT_TOT)), _(label)) {}

MI_CRASHES_X_LAST::MI_CRASHES_X_LAST()
    : WI_INFO_t(crash_s.counter_crash.x, _(label)) {}

MI_CRASHES_Y_LAST::MI_CRASHES_Y_LAST()
    : WI_INFO_t(crash_s.counter_crash.y, _(label)) {}

MI_CRASHES_X::MI_CRASHES_X()
    : WI_INFO_t(variant8_get_ui16(eeprom_get_var(EEVAR_CRASH_COUNT_X_TOT)), _(label)) {}

MI_CRASHES_Y::MI_CRASHES_Y()
    : WI_INFO_t(variant8_get_ui16(eeprom_get_var(EEVAR_CRASH_COUNT_Y_TOT)), _(label)) {}

    #if HAS_DRIVER(TMC2130)
MI_CRASH_FILTERING::MI_CRASH_FILTERING()
    : WI_ICON_SWITCH_OFF_ON_t(0, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
    index = crash_s.get_filter();
}

void MI_CRASH_FILTERING::OnChange(size_t old_index) {
    crash_s.set_filter(index);
}
    #endif
#endif // ANY(CRASH_RECOVERY, POWER_PANIC)
