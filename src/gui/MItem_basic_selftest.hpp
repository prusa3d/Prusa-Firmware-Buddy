/**
 * @file MItem_basic_selftest.hpp
 * @author Radek Vana
 * @brief general (all printer types) selftest menu items
 * @date 2021-10-04
 */

#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "WindowItemFormatableLabel.hpp"
#include "WindowItemFanLabel.hpp"
#include "WindowItemTempLabel.hpp"
#include <option/has_toolchanger.h>
#include <option/filament_sensor.h>
#include <option/has_mmu2.h>

class MI_WIZARD : public WI_LABEL_t {
    static constexpr const char *const label = N_("Wizard");

public:
    MI_WIZARD();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SELFTEST : public WI_LABEL_t {
    static constexpr const char *const label = N_("Run Full Selftest");

public:
    MI_SELFTEST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SELFTEST_RESULT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Show Selftest Result");

public:
    MI_SELFTEST_RESULT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_CALIB_FIRST : public WI_LABEL_t {
    static constexpr const char *const label = N_("First Layer Calibration");

public:
    MI_CALIB_FIRST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_FANS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Test Fans");

public:
    MI_TEST_FANS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_XYZ : public WI_LABEL_t {
    static constexpr const char *const label = N_("Test XYZ-Axis");

public:
    MI_TEST_XYZ();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_X : public WI_LABEL_t {
    static constexpr const char *const label = "Test X-Axis"; // dev mode - not translated

public:
    MI_TEST_X();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_Y : public WI_LABEL_t {
    static constexpr const char *const label = "Test Y-Axis"; // dev mode - not translated

public:
    MI_TEST_Y();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_Z : public WI_LABEL_t {
    static constexpr const char *const label = "Test Z-Axis"; // dev mode - not translated

public:
    MI_TEST_Z();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_HEAT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Test Heaters");

public:
    MI_TEST_HEAT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_HOTEND : public WI_LABEL_t {
    static constexpr const char *const label = "Test Hotend"; // debug only - not translated

public:
    MI_TEST_HOTEND();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_BED : public WI_LABEL_t {
    static constexpr const char *const label = "Test Bed"; // debug only - not translated

public:
    MI_TEST_BED();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

#if FILAMENT_SENSOR_IS_ADC()
class MI_CALIB_FSENSOR : public WI_LABEL_t {
    static constexpr const char *const label = N_("Filament Sensor Calibration");

public:
    MI_CALIB_FSENSOR();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

    #if HAS_MMU2()
class MI_CALIB_FSENSOR_MMU : public WI_LABEL_t {
    static constexpr const char *const label = N_("Filament Sensor Calibration MMU");

public:
    MI_CALIB_FSENSOR_MMU();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
    #endif
#endif

#if PRINTER_IS_PRUSA_MK4
class MI_CALIB_GEARS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Gears Calibration");

public:
    MI_CALIB_GEARS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
#endif

#if HAS_TOOLCHANGER()
class MI_CALIBRATE_TOOL_OFFSETS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Calibrate Tool Offsets");

public:
    MI_CALIBRATE_TOOL_OFFSETS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_RESTORE_CALIBRATION_FROM_USB : public WI_LABEL_t {
    static constexpr const char *const label = N_("Restore Calibration from USB");

public:
    MI_RESTORE_CALIBRATION_FROM_USB();

protected:
    virtual void click(IWindowMenu &window_menu) override;

private:
    bool restore_fs_calibration();
};

class MI_BACKUP_CALIBRATION_TO_USB : public WI_LABEL_t {
    static constexpr const char *const label = N_("Backup Calibration to USB");

public:
    MI_BACKUP_CALIBRATION_TO_USB();

protected:
    virtual void click(IWindowMenu &window_menu) override;

private:
    bool backup_fs_calibration();
};
#endif
