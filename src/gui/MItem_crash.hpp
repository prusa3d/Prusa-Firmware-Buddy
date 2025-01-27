/**
 * @file MItem_crash.hpp
 * @author Radek Vana
 * @brief menu items for crash recovery
 * @date 2021-11-05
 */

#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"

class MI_CRASH_DETECTION : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Crash Detection");

public:
    MI_CRASH_DETECTION();
    virtual void Loop() override;
    virtual void OnChange(size_t old_index) override;
};

class MI_CRASH_SENSITIVITY_X : public WiSpin {
private:
    constexpr static const char *const label = "Crash Sensitivity X";

public:
    MI_CRASH_SENSITIVITY_X();
    virtual void OnClick() override;
};

class MI_CRASH_SENSITIVITY_Y : public WiSpin {
private:
    constexpr static const char *const label = "Crash Sensitivity Y";

public:
    MI_CRASH_SENSITIVITY_Y();
    virtual void OnClick() override;
};

#if PRINTER_IS_PRUSA_XL
// XL set Crash Sensitivity in user friendly was (Low/Medium/High), whereas other printers set integer directly and its development menu only

class MI_CRASH_SENSITIVITY_XY : public WI_SWITCH_t<3> {
private:
    constexpr static const char *const label = N_("Crash Sensitivity XY");

    struct item_t {
        const char *name;
        uint8_t value;
    };

    constexpr static item_t ITEMS[3] = {
        { N_("Low"), 3 },
        { N_("Medium"), 2 },
        { N_("High"), 1 },
    };
    constexpr size_t get_item_id_from_sensitivity(int32_t sensitivity);

public:
    MI_CRASH_SENSITIVITY_XY();
    virtual void OnChange(size_t old_index) override;
};
#else
class MI_CRASH_SENSITIVITY_XY : public WiSpin {
private:
    constexpr static const char *const label = N_("Crash Sensitivity XY");

public:
    MI_CRASH_SENSITIVITY_XY();
    virtual void OnClick() override;
};
#endif

#if ANY(CRASH_RECOVERY, POWER_PANIC)

class MI_CRASH_MAX_PERIOD_X : public WiSpin {
private:
    constexpr static const char *const label = "Crash Max. Period X";

public:
    MI_CRASH_MAX_PERIOD_X();
    virtual void OnClick() override;
};

class MI_CRASH_MAX_PERIOD_Y : public WiSpin {
private:
    constexpr static const char *const label = "Crash Max. Period Y";

public:
    MI_CRASH_MAX_PERIOD_Y();
    virtual void OnClick() override;
};

class MI_POWER_PANICS : public WI_INFO_t {
    constexpr static const char *const label = N_("Power Failures");

public:
    MI_POWER_PANICS();
};

class MI_CRASHES_X_LAST : public WI_INFO_t {
    constexpr static const char *const label = N_("Last Print Crashes on X Axis");

public:
    MI_CRASHES_X_LAST();
};

class MI_CRASHES_Y_LAST : public WI_INFO_t {
    constexpr static const char *const label = N_("Last Print Crashes on Y Axis");

public:
    MI_CRASHES_Y_LAST();
};

class MI_CRASHES_X : public WI_INFO_t {
    constexpr static const char *const label = N_("Crashes on X Axis");

public:
    MI_CRASHES_X();
};

class MI_CRASHES_Y : public WI_INFO_t {
    constexpr static const char *const label = N_("Crashes on Y Axis");

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
