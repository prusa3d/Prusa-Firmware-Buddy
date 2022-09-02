/**
 * @file MItem_crash.hpp
 * @author Radek Vana
 * @brief menu items for crash recovery
 * @date 2021-11-05
 */

#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"

class MI_CRASH_DETECTION : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Crash Detection");

public:
    MI_CRASH_DETECTION();
    virtual void OnChange(size_t old_index) override;
};

class MI_CRASH_SENSITIVITY_X : public WiSpinInt {
private:
    constexpr static const char *const label = "Crash Sensitivity X";

public:
    MI_CRASH_SENSITIVITY_X();
    virtual void OnClick() override;
};

class MI_CRASH_SENSITIVITY_Y : public WiSpinInt {
private:
    constexpr static const char *const label = "Crash Sensitivity Y";

public:
    MI_CRASH_SENSITIVITY_Y();
    virtual void OnClick() override;
};

class MI_CRASH_MAX_PERIOD_X : public WI_SPIN_CRASH_PERIOD_t {
private:
    constexpr static const char *const label = "Crash Min. Speed X";

public:
    MI_CRASH_MAX_PERIOD_X();
    virtual void OnClick() override;
};

class MI_CRASH_MAX_PERIOD_Y : public WI_SPIN_CRASH_PERIOD_t {
private:
    constexpr static const char *const label = "Crash Min. Speed Y";

public:
    MI_CRASH_MAX_PERIOD_Y();
    virtual void OnClick() override;
};

#if ANY(CRASH_RECOVERY, POWER_PANIC)
class MI_POWER_PANICS : public WI_INFO_t {
    constexpr static const char *const label = N_("Power failures");

public:
    MI_POWER_PANICS();
};

class MI_CRASHES_X_LAST : public WI_INFO_t {
    constexpr static const char *const label = N_("Last print crashes on X axis");

public:
    MI_CRASHES_X_LAST();
};

class MI_CRASHES_Y_LAST : public WI_INFO_t {
    constexpr static const char *const label = N_("Last print crashes on Y axis");

public:
    MI_CRASHES_Y_LAST();
};

class MI_CRASHES_X : public WI_INFO_t {
    constexpr static const char *const label = N_("Crashes on X axis");

public:
    MI_CRASHES_X();
};

class MI_CRASHES_Y : public WI_INFO_t {
    constexpr static const char *const label = N_("Crashes on Y axis");

public:
    MI_CRASHES_Y();
};
    #if HAS_DRIVER(TMC2130)
class MI_CRASH_FILTERING : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Crash Detection Filter");

public:
    MI_CRASH_FILTERING();
    virtual void OnChange(size_t old_index) override;
};
    #endif
#endif // ANY(CRASH_RECOVERY, POWER_PANIC)
