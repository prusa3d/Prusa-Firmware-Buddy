/*****************************************************************************/
// Screen openning menu items
#pragma once
#include "WindowMenuItems.hpp"
#include "WindowMenuSwitch.hpp"
#include "i18n.h"
#include <option/has_toolchanger.h>
#include <option/has_side_leds.h>
#include <option/has_leds.h>
#include <option/developer_mode.h>
#include <common/sheet.hpp>

class MI_VERSION_INFO : public WI_LABEL_t {
    static constexpr const char *const label = N_("Version Info");

public:
    MI_VERSION_INFO();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SENSOR_INFO : public WI_LABEL_t {
    static constexpr const char *const label = N_("Sensor Info");

public:
    MI_SENSOR_INFO();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_ODOMETER : public WI_LABEL_t {
    static constexpr const char *const label = N_("Statistics");

public:
    MI_ODOMETER();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_FILAMENT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Filament");

public:
    MI_FILAMENT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SYS_INFO : public WI_LABEL_t {
    static constexpr const char *const label = N_("System Info");

public:
    MI_SYS_INFO();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_STATISTIC_disabled : public WI_LABEL_t {
    static constexpr const char *const label = N_("Statistic");

public:
    MI_STATISTIC_disabled();

protected:
    virtual void click([[maybe_unused]] IWindowMenu &window_menu) override {}
};

class MI_FAIL_STAT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Fail Stats");

public:
    MI_FAIL_STAT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SUPPORT_disabled : public WI_LABEL_t {
    static constexpr const char *const label = N_("Support");

public:
    MI_SUPPORT_disabled();

protected:
    virtual void click([[maybe_unused]] IWindowMenu &window_menu) override {}
};

class MI_TEMPERATURE : public WI_LABEL_t {
    static constexpr const char *const label = N_("Temperature");

public:
    MI_TEMPERATURE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MOVE_AXIS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Move Axis");

public:
    MI_MOVE_AXIS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SERVICE : public WI_LABEL_t {
    static constexpr const char *const label = N_("Service");

public:
    MI_SERVICE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

#if DEVELOPER_MODE()
/**
 * @brief Test Errors as BSOD, redscreens, watchdog and so on.
 * @note Enabled only in developer mode. Can be used in release build.
 */
class MI_ERROR_TEST : public WI_LABEL_t {
    static constexpr const char *const label = N_("Test Errors");

public:
    MI_ERROR_TEST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
#endif /*DEVELOPMENT_ITEMS()*/

class MI_TEST : public WI_LABEL_t {
    static constexpr const char *const label = N_("Test");

public:
    MI_TEST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_FW_UPDATE : public WI_LABEL_t {
    static constexpr const char *const label = N_("FW Update");

public:
    MI_FW_UPDATE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_METRICS_SETTINGS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Metrics & Log");

public:
    MI_METRICS_SETTINGS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_ETH_SETTINGS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Ethernet");

public:
    MI_ETH_SETTINGS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_WIFI_SETTINGS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Wi-Fi");

public:
    MI_WIFI_SETTINGS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MESSAGES : public WI_LABEL_t {
    static constexpr const char *const label = N_("Message History");

public:
    MI_MESSAGES();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_PRUSA_CONNECT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Prusa Connect");

public:
    MI_PRUSA_CONNECT();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_HW_SETUP : public WI_LABEL_t {
    static constexpr const char *const label = N_("HW Setup");

public:
    MI_HW_SETUP();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_CURRENT_PROFILE : public WI_LABEL_t {
    static constexpr const char *const label = N_("Current Profile");
    char name[MAX_SHEET_NAME_LENGTH + 3];

public:
    MI_CURRENT_PROFILE();

    void UpdateLabel();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EEPROM : public WI_LABEL_t {
    static constexpr const char *const label = "Eeprom";

public:
    MI_EEPROM();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_DEVHASH_IN_QR : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Device Hash in QR");

public:
    MI_DEVHASH_IN_QR();
    virtual void OnChange(size_t old_index) override;
};

class MI_FOOTER_SETTINGS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Footer");

public:
    MI_FOOTER_SETTINGS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_WAVETABLE_XYZ : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Change Wave Table XYZ");

public:
    MI_WAVETABLE_XYZ();
    virtual void OnChange(size_t old_index) override;
};

class MI_FOOTER_SETTINGS_ADV : public WI_LABEL_t {
    static constexpr const char *const label = N_("Advanced");

public:
    MI_FOOTER_SETTINGS_ADV();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EXPERIMENTAL_SETTINGS : public WI_LABEL_t {
    static constexpr const char *const label = "Experimental Settings";

public:
    MI_EXPERIMENTAL_SETTINGS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EEPROM_DIAGNOSTICS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Eeprom Diagnostics");

public:
    MI_EEPROM_DIAGNOSTICS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_DIAGNOSTICS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Diagnostics");

public:
    MI_DIAGNOSTICS();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_USER_INTERFACE : public WI_LABEL_t {
    static constexpr const char *const label = N_("User Interface");

public:
    MI_USER_INTERFACE();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_LANG_AND_TIME : public WI_LABEL_t {
    static constexpr const char *const label = N_("Language & Time");

public:
    MI_LANG_AND_TIME();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_LOAD_SETTINGS : public WI_LABEL_t {
    constexpr static const char *const label = N_("Load Settings from File");

public:
    MI_LOAD_SETTINGS();

    virtual void click(IWindowMenu &) override;
};

class MI_NETWORK : public WI_LABEL_t {
    static constexpr const char *const label = N_("Network");

public:
    MI_NETWORK();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_HARDWARE : public WI_LABEL_t {
    static constexpr const char *const label = N_("Hardware");

public:
    MI_HARDWARE();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_HARDWARE_TUNE : public WI_LABEL_t {
    static constexpr const char *const label = N_("Hardware");

public:
    MI_HARDWARE_TUNE();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_SYSTEM : public WI_LABEL_t {
    static constexpr const char *const label = N_("System");

public:
    MI_SYSTEM();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_PRINT_STATISTICS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Print Statistics");

public:
    MI_PRINT_STATISTICS();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_INFO : public WI_LABEL_t {
    static constexpr const char *const label = N_("Info");

public:
    MI_INFO();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_PRUSALINK : public WI_LABEL_t {
    static constexpr const char *const label = "PrusaLink";

public:
    MI_PRUSALINK();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_USB_MSC_ENABLE : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static char const *label = "USB MSC";

public:
    MI_USB_MSC_ENABLE();
    virtual void OnChange(size_t old_index) override;
};
#if HAS_LEDS()
class MI_LEDS_ENABLE : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("RGB Status Bar");

public:
    MI_LEDS_ENABLE();
    virtual void OnChange(size_t old_index) override;
};
#endif

#if HAS_SIDE_LEDS()
class MI_SIDE_LEDS_ENABLE : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("RGB Side Strip");

public:
    MI_SIDE_LEDS_ENABLE();
    virtual void OnChange(size_t old_index) override;
};
#endif

#if HAS_TOOLCHANGER()
class MI_TOOL_LEDS_ENABLE : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Tool Light");

public:
    MI_TOOL_LEDS_ENABLE();
    virtual void OnChange(size_t old_index) override;
};
#endif /*HAS_TOOLCHANGER()*/

class MI_TOOLS_SETUP : public WI_LABEL_t {
    static constexpr const char *const label = N_("Tools");

public:
    MI_TOOLS_SETUP();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TRIGGER_POWER_PANIC : public WI_LABEL_t {
    static constexpr const char *const label = N_("Trigger Power Panic");

public:
    MI_TRIGGER_POWER_PANIC();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_PICK_PARK_TOOL : public WI_LABEL_t {
    static constexpr const char *const label = N_("Pick/Park Tool");

public:
    MI_PICK_PARK_TOOL();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_CALIBRATE_DOCK : public WI_LABEL_t {
    static constexpr const char *const label = N_("Calibrate Dock Position");

public:
    MI_CALIBRATE_DOCK();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SELFTEST_SNAKE : public WI_LABEL_t {
    static constexpr const char *const label = N_("Calibrations & Tests");

public:
    MI_SELFTEST_SNAKE();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_OPEN_FACTORY_RESET : public WI_LABEL_t {
    static constexpr const char *const label = N_("Factory Reset");

public:
    MI_OPEN_FACTORY_RESET();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_INPUT_SHAPER : public WI_LABEL_t {
    constexpr static const char *label = N_("Input Shaper");

public:
    MI_INPUT_SHAPER();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};
