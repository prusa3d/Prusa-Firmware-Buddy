/*****************************************************************************/
// menu items running tools
#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "filament.hpp"
#include "WindowItemFormatableLabel.hpp"
#include "WindowItemFanLabel.hpp"
#include "config.h"
#include <buddy/door_sensor.hpp>
#include <common/filament_sensor.hpp>
#include <common/filament_sensor_states.hpp>
#include <utility_extensions.hpp>
#include <option/has_door_sensor.h>
#include <option/has_dwarf.h>
#include <option/has_filament_sensors_menu.h>
#include <option/has_coldpull.h>
#include <option/has_leds.h>
#include <option/has_phase_stepping_toggle.h>
#include <option/has_side_leds.h>
#include <option/buddy_enable_connect.h>
#include <option/has_belt_tuning.h>
#include <trinamic.h>
#include <meta_utils.hpp>
#include <str_utils.hpp>
#include <gui/menu_item/menu_item_gcode_action.hpp>

/// \returns tool name for tool menu item purposes
inline constexpr const char *tool_name(uint8_t tool_index) {
    switch (tool_index) {
    case 0:
        return N_("Tool 1");
    case 1:
        return N_("Tool 2");
    case 2:
        return N_("Tool 3");
    case 3:
        return N_("Tool 4");
    case 4:
        return N_("Tool 5");
    default:
        return "";
    }
}

/// Checks if there is space in the gcode queue for inserting further commands.
/// If there's not, \returns false and shows a message box
bool gui_check_space_in_gcode_queue_with_msg();

/// Attempts to execute the gcode.
/// \returns false on failure (when the queue is full) and shows a message box saying the printer is busy
bool gui_try_gcode_with_msg(const char *gcode);

/// Global filamen sensing enable/disable
class MI_FILAMENT_SENSOR : public WI_ICON_SWITCH_OFF_ON_t {
    // If the printer has filament sensors menu, this item is inside it and is supposed to be called differently (BFW-4973)
    static constexpr const char *const label = HAS_FILAMENT_SENSORS_MENU() ? N_("Filament Sensing") : N_("Filament Sensor");

public:
    MI_FILAMENT_SENSOR();

    /// Set the index to the correct value based on config_store
    void update();

protected:
    virtual void OnChange(size_t old_index) override;
};

class MI_STUCK_FILAMENT_DETECTION : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Stuck Filament Detection");
    bool init_index() const;

public:
    MI_STUCK_FILAMENT_DETECTION()
        : WI_ICON_SWITCH_OFF_ON_t(init_index(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void OnChange(size_t old_index) override;
};

class MI_STEALTH_MODE : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Stealth Mode");

public:
    MI_STEALTH_MODE(); // @@TODO probably XL only

protected:
    virtual void OnChange(size_t old_index) override;
};

class MI_LIVE_ADJUST_Z : public IWindowMenuItem {
    static constexpr const char *const label = N_("Live Adjust Z");

public:
    MI_LIVE_ADJUST_Z();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_AUTO_HOME : public IWindowMenuItem {
public:
    MI_AUTO_HOME();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MESH_BED : public IWindowMenuItem {
    static constexpr const char *const label = N_("Mesh Bed Leveling");

public:
    MI_MESH_BED();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_DISABLE_STEP : public IWindowMenuItem {
    static constexpr const char *const label = N_("Disable Motors");

public:
    MI_DISABLE_STEP();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_FACTORY_SOFT_RESET : public IWindowMenuItem {
    static constexpr const char *const label = N_("Reset Settings & Calibrations");

public:
    MI_FACTORY_SOFT_RESET();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_FACTORY_HARD_RESET : public IWindowMenuItem {
    static constexpr const char *const label = N_("Hard Reset (USB with FW needed)");

public:
    MI_FACTORY_HARD_RESET();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

#if PRINTER_IS_PRUSA_MK4()
class MI_FACTORY_SHIPPING_PREP : public IWindowMenuItem {
    static constexpr const char *const label = N_("Shipping Preparation");

public:
    MI_FACTORY_SHIPPING_PREP();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
#endif

#ifdef BUDDY_ENABLE_DFU_ENTRY
class MI_ENTER_DFU : public IWindowMenuItem {
    static constexpr const char *const label = "Enter DFU";

public:
    MI_ENTER_DFU();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
#endif

class MI_SAVE_DUMP : public IWindowMenuItem {
    static constexpr const char *const label = N_("Save Crash Dump");

public:
    MI_SAVE_DUMP();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_XFLASH_RESET : public IWindowMenuItem {
    static constexpr const char *const label = "Delete Crash Dump"; // intentionally not translated, only for debugging

public:
    MI_XFLASH_RESET();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_SAVEXML : public IWindowMenuItem {
    static constexpr const char *const label = "TODO EE Save XML"; // intentionally not translated, only for debugging

public:
    MI_EE_SAVEXML();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_CLEAR : public IWindowMenuItem {
    static constexpr const char *const label = "EE Clear"; // intentionally not translated, only for debugging

public:
    MI_EE_CLEAR();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_M600 : public IWindowMenuItem {
    static constexpr const char *const label = N_("Change Filament");
    bool enqueued = false; // Used to avoid multiple M600 enqueue
public:
    MI_M600();
    void resetEnqueued() { enqueued = false; }

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_DRYRUN : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Dry run (no extrusion)");

public:
    MI_DRYRUN();

protected:
    virtual void OnChange(size_t old_index) override;
};

class MI_TIMEOUT : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Menu Timeout");

public:
    MI_TIMEOUT();
    virtual void OnChange(size_t old_index) override;
};

#ifdef _DEBUG
inline constexpr size_t MI_SOUND_MODE_COUNT = 5;
#else
inline constexpr size_t MI_SOUND_MODE_COUNT = 4;
#endif
class MI_SOUND_MODE : public MenuItemSwitch {
    constexpr static const char *const label = N_("Sound Mode");

    size_t init_index() const;

public:
    MI_SOUND_MODE();
    virtual void OnChange(size_t old_index) override;
};

class MI_SORT_FILES : public MenuItemSwitch {
public:
    MI_SORT_FILES();
    virtual void OnChange(size_t old_index) override;
};

class MI_SOUND_VOLUME : public WiSpin {
    constexpr static const char *const label = N_("Sound Volume");

public:
    MI_SOUND_VOLUME();
    virtual void OnClick() override;
    /* virtual void Change() override; */
};

class MI_TIMEZONE : public WiSpin {
    constexpr static const char *const label = N_("Time Zone Hour Offset");

public:
    MI_TIMEZONE();
    virtual void OnClick() override;
};

class MI_TIMEZONE_MIN : public MenuItemSwitch {
public:
    MI_TIMEZONE_MIN();
    virtual void OnChange(size_t old_index) override;
};

class MI_TIMEZONE_SUMMER : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Time Zone Summertime");

public:
    MI_TIMEZONE_SUMMER();
    virtual void OnChange(size_t old_index) override;
};

class MI_TIME_FORMAT : public MenuItemSwitch {

public:
    MI_TIME_FORMAT();
    virtual void OnChange(size_t old_index) override;
};

/**
 * @brief Menu item with current time.
 * @warning This uses time_tools::get_time() which needs to be updated periodically.
 *     It needs to be done from the menu which has windowEvent() method.
 */
class MI_TIME_NOW : public WiInfo<8> {
public:
    MI_TIME_NOW();
};

class MI_FAN_CHECK : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Fan Check");

public:
    MI_FAN_CHECK();
    virtual void OnChange(size_t old_index) override;
};

/******************************************************************/
// WI_INFO_t
class MI_INFO_FW : public WI_INFO_t {
    static constexpr const char *const label = N_("Firmware Version");

public:
    MI_INFO_FW();
    virtual void click(IWindowMenu &window_menu) override; // FW version is clickable when development tools are shown
};

class MI_INFO_BOOTLOADER : public WI_INFO_t {
    static constexpr const char *const label = N_("Bootloader Version");

public:
    MI_INFO_BOOTLOADER();
};

class MI_INFO_MMU : public WI_INFO_t {
    static constexpr const char *const label = N_("MMU Version");

public:
    MI_INFO_MMU();
};

class MI_INFO_BOARD : public WI_INFO_t {
    static constexpr const char *const label = N_("Buddy Board");

public:
    MI_INFO_BOARD();
};

class MI_INFO_SERIAL_NUM : public WiInfo<28> {
    static constexpr const char *const label = N_("Serial Number");

public:
    MI_INFO_SERIAL_NUM();
};

class MI_FS_AUTOLOAD : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Filament Autoloading");

public:
    MI_FS_AUTOLOAD();
    virtual void OnChange(size_t old_index) override;
};

class MI_INFO_BED_TEMP : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_BED_TEMP();
};

class MI_INFO_FILAMENT_SENSOR : public MenuItemAutoUpdatingLabel<FilamentSensorStateAndValue> {
public:
    MI_INFO_FILAMENT_SENSOR(const string_view_utf8 &label, const GetterFunction &getter_function);

protected:
    void print_val(const std::span<char> &buffer) const;
    static FilamentSensorStateAndValue get_value(IFSensor *fsensor);
};

class MI_INFO_PRINTER_FILL_SENSOR : public MI_INFO_FILAMENT_SENSOR {
public:
    MI_INFO_PRINTER_FILL_SENSOR();
};

class MI_INFO_SIDE_FILL_SENSOR : public MI_INFO_FILAMENT_SENSOR {
public:
    MI_INFO_SIDE_FILL_SENSOR();
};

class MI_INFO_PRINT_FAN : public WI_FAN_LABEL_t {
public:
    MI_INFO_PRINT_FAN();
};

class MI_INFO_HBR_FAN : public WI_FAN_LABEL_t {
public:
    MI_INFO_HBR_FAN();
};

class MI_PRINT_PROGRESS_TIME : public WiSpin {

public:
    constexpr static const char *label = N_("Print Progress Screen");

    static constexpr NumericInputConfig config {
        .min_value = 30,
        .max_value = 200,
        .special_value = 29,
        .unit = Unit::second,
    };

public:
    MI_PRINT_PROGRESS_TIME();

protected:
    virtual void OnClick() override;
};

class MI_ODOMETER_DIST : public WI_FORMATABLE_LABEL_t<float> {
public:
    MI_ODOMETER_DIST(const string_view_utf8 &label, const img::Resource *icon, is_enabled_t enabled, is_hidden_t hidden, float initVal);
};

class MI_ODOMETER_DIST_X : public MI_ODOMETER_DIST {
    constexpr static const char *const label = N_("X Axis");

public:
    MI_ODOMETER_DIST_X();
};
class MI_ODOMETER_DIST_Y : public MI_ODOMETER_DIST {
    constexpr static const char *const label = N_("Y Axis");

public:
    MI_ODOMETER_DIST_Y();
};
class MI_ODOMETER_DIST_Z : public MI_ODOMETER_DIST {
    constexpr static const char *const label = N_("Z Axis");

public:
    MI_ODOMETER_DIST_Z();
};

/// Extruded filament
class MI_ODOMETER_DIST_E : public MI_ODOMETER_DIST {
    constexpr static const char *const generic_label = N_("Filament");

public:
    MI_ODOMETER_DIST_E(const char *const label, int index);
    MI_ODOMETER_DIST_E();
};

/// Tool picked
class MI_ODOMETER_TOOL : public WI_FORMATABLE_LABEL_t<uint32_t> {
    constexpr static const char *const generic_label = N_("Tools Changed");
    constexpr static const char *const times_label = N_("times"); // Tools Changed      123 times

public:
    MI_ODOMETER_TOOL(const char *const label, int index);
    MI_ODOMETER_TOOL();
};

class MI_ODOMETER_MMU_CHANGES : public WI_FORMATABLE_LABEL_t<uint32_t> {
    constexpr static const char *const label = N_("MMU filament loads");

public:
    MI_ODOMETER_MMU_CHANGES();
};

class MI_ODOMETER_TIME : public WI_FORMATABLE_LABEL_t<uint32_t> {
    constexpr static const char *const label = N_("Print Time");

public:
    MI_ODOMETER_TIME();
};

#if BOARD_IS_XBUDDY()
class MI_INFO_HEATER_VOLTAGE : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_HEATER_VOLTAGE();
};

class MI_INFO_HEATER_CURRENT : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_HEATER_CURRENT();
};

class MI_INFO_INPUT_CURRENT : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_INPUT_CURRENT();
};

class MI_INFO_MMU_CURRENT : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_MMU_CURRENT();
};
#endif

#if BOARD_IS_XLBUDDY()
class MI_INFO_5V_VOLTAGE : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_5V_VOLTAGE();
};

class MI_INFO_SANDWICH_5V_CURRENT : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_SANDWICH_5V_CURRENT();
};

class MI_INFO_BUDDY_5V_CURRENT : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_BUDDY_5V_CURRENT();
};
#endif

class MI_INFO_INPUT_VOLTAGE : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_INPUT_VOLTAGE();
};

class MI_INFO_BOARD_TEMP : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_BOARD_TEMP();
};

#if HAS_DOOR_SENSOR()
class MI_INFO_DOOR_SENSOR : public MenuItemAutoUpdatingLabel<buddy::DoorSensor::DetailedState> {
private:
    void print_val(const std::span<char> &buffer) const;

public:
    MI_INFO_DOOR_SENSOR();
};
#endif

class MI_INFO_MCU_TEMP final : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_MCU_TEMP();
};

class MI_FOOTER_RESET : public IWindowMenuItem {
    static constexpr const char *const label = N_("Reset");

public:
    MI_FOOTER_RESET();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_CO_CANCEL_OBJECT : public IWindowMenuItem {
    static constexpr const char *const label = N_("Cancel Object");

public:
    MI_CO_CANCEL_OBJECT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_HEATUP_BED : public MenuItemSwitch {
public:
    MI_HEATUP_BED();

protected:
    void OnChange(size_t old_index) override;
};

/******************************************************************/

class MI_SET_READY : public IWindowMenuItem {
    static constexpr const char *const label = N_("Set Ready");

public:
    MI_SET_READY();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

#if HAS_PHASE_STEPPING_TOGGLE()

class MI_PHASE_STEPPING_TOGGLE : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *label = "Phase Stepping";
    bool event_in_progress { false };

public:
    MI_PHASE_STEPPING_TOGGLE();

protected:
    void OnChange(size_t old_index) override;
};

#endif

/******************************************************************/
#if HAS_COLDPULL()

class MI_COLD_PULL : public IWindowMenuItem {
    static constexpr const char *const label = N_("Cold Pull");

public:
    MI_COLD_PULL();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

#endif

class MI_GCODE_VERIFY : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Verify GCode");

public:
    MI_GCODE_VERIFY();
    virtual void OnChange(size_t old_index) override;
};

class MI_DEVHASH_IN_QR : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Device Hash in QR");

public:
    MI_DEVHASH_IN_QR();
    virtual void OnChange(size_t old_index) override;
};

class MI_WAVETABLE_XYZ : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Change Wave Table XYZ");

public:
    MI_WAVETABLE_XYZ();
    virtual void OnChange(size_t old_index) override;
};

class MI_LOAD_SETTINGS : public IWindowMenuItem {
    constexpr static const char *const label = N_("Load Settings from File");

public:
    MI_LOAD_SETTINGS();

    virtual void click(IWindowMenu &) override;
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

class MI_DISPLAY_BACKLIGHT_BRIGHTNESS : public WiSpin {
    static constexpr const char *const label = N_("Display Backlight Brightness");

public:
    MI_DISPLAY_BACKLIGHT_BRIGHTNESS();
    virtual void OnClick() override;
};
#endif

#if HAS_SIDE_LEDS()
class MI_SIDE_LEDS_ENABLE : public WiSpin {

    static constexpr const char *const label =
    #if PRINTER_IS_PRUSA_COREONE()
        N_("Chamber Lights");
    #else
        N_("RGB Side Strip");
    #endif

public:
    MI_SIDE_LEDS_ENABLE();
    virtual void OnClick() override;
};

class MI_SIDE_LEDS_DIMMING : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label =
    #if PRINTER_IS_PRUSA_COREONE()
        N_("Chamber Dimming");
    #else
        N_("RGB Side Strip Dimming");
    #endif

public:
    MI_SIDE_LEDS_DIMMING();
    virtual void OnChange(size_t old_index) override;
};

class MI_SIDE_LEDS_DIMMING_DURATION : public WiSpin {
    static constexpr const char *const label = N_("RGB Side Strip Dimming Duration");

public:
    MI_SIDE_LEDS_DIMMING_DURATION();
    virtual void OnClick() override;
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

#if HAS_TOOLCHANGER()
class MI_TOOL_LEDS_BRIGHTNESS : public WiSpin {
    static constexpr const char *const label = N_("Tool Light Brightness");

public:
    MI_TOOL_LEDS_BRIGHTNESS();
    virtual void OnClick() override;
};
#endif /*HAS_TOOLCHANGER()*/

#if ENABLED(POWER_PANIC)
class MI_TRIGGER_POWER_PANIC : public IWindowMenuItem {
    static constexpr const char *const label = N_("Trigger Power Panic");

public:
    MI_TRIGGER_POWER_PANIC();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};
#endif

#if HAS_TOOLCHANGER()
class MI_PICK_PARK_TOOL : public IWindowMenuItem {
    static constexpr const char *const label = N_("Pick/Park Tool");

public:
    MI_PICK_PARK_TOOL();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_CALIBRATE_DOCK : public IWindowMenuItem {
    static constexpr const char *const label = N_("Calibrate Dock Position");

public:
    MI_CALIBRATE_DOCK();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
#endif

#if HAS_BELT_TUNING()
using MI_BELT_TUNING = WithConstructorArgs<MenuItemGcodeAction, N_("Belt Tuning"), "M960 W"_tstr>;
#endif

#if HAS_ILI9488_DISPLAY()
class MI_DISPLAY_BAUDRATE : public MenuItemSwitch {
public:
    MI_DISPLAY_BAUDRATE();
    virtual void OnChange(size_t old_index) override;
};
#endif

/// Useless menu item that is empty and always hidden.
/// Used as an endstop for trailing commas
class MI_ALWAYS_HIDDEN : public IWindowMenuItem {
public:
    MI_ALWAYS_HIDDEN()
        : IWindowMenuItem({}, nullptr, is_enabled_t::no, is_hidden_t::yes) {}
};

class MI_LOG_TO_TXT : public WI_ICON_SWITCH_OFF_ON_t {
public:
    MI_LOG_TO_TXT();
    void OnChange(size_t) final;
};
