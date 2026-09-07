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
#include <feature/filament_sensor/filament_sensor.hpp>
#include <feature/filament_sensor/filament_sensor_states.hpp>
#include <utility_extensions.hpp>
#include <option/has_door_sensor.h>
#include <option/has_dwarf.h>
#include <option/has_indx.h>
#include <option/has_filament_sensors_menu.h>
#include <option/has_coldpull.h>
#include <option/has_leds.h>
#include <option/has_power_panic.h>
#include <option/has_side_leds.h>
#include <option/buddy_enable_connect.h>
#include <option/has_auto_retract.h>
#include <option/has_toolchanger.h>
#include <option/has_indx.h>
#include <option/has_wastebin_fill_tracking.h>
#include <option/has_extruder_fsensor.h>
#include <meta_utils.hpp>
#include <gui/menu_item/menu_item_gcode_action.hpp>

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

#if !HAS_INDX()
class MI_STUCK_FILAMENT_DETECTION : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Stuck Filament Detection");
    bool init_index() const;

public:
    MI_STUCK_FILAMENT_DETECTION()
        : WI_ICON_SWITCH_OFF_ON_t(init_index(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void OnChange(size_t old_index) override;
};
#endif

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

#if HAS_WASTEBIN_FILL_TRACKING()
/// Tune / Wastebin-submenu action: park the nozzle clear of the INDX wastebin so the user can empty
/// it, then reset the pellet fill counter (handled by M1986). Works both mid-print and while idle.
class MI_NOZZLE_CLEANER_EMPTY_WASTEBIN : public IWindowMenuItem {
    static constexpr const char *const label = N_("Empty Nozzle Cleaner");

public:
    MI_NOZZLE_CLEANER_EMPTY_WASTEBIN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
    virtual void Loop() override;
};

/// Wastebin submenu: auto-pause the print when the nozzle-cleaner wastebin reaches capacity
/// (otherwise only a non-blocking warning is shown).
class MI_NOZZLE_CLEANER_AUTOPAUSE : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Pause On Full Nozzle Cleaner");

public:
    MI_NOZZLE_CLEANER_AUTOPAUSE();

protected:
    void OnChange(size_t old_index) override;
};

/// Wastebin submenu: which nozzle-cleaner is installed - standard or extended (high-capacity)
/// wastebin. Selects the capacity used for overfill warnings.
class MI_NOZZLE_CLEANER_CAPACITY : public MenuItemSwitch {
    static constexpr const char *const label = N_("Nozzle Cleaner Capacity");

public:
    MI_NOZZLE_CLEANER_CAPACITY();

protected:
    void OnChange(size_t old_index) final;
};

/// Wastebin submenu: read-only fill level (pellets ejected since last emptied / capacity).
class MI_NOZZLE_CLEANER_FILL : public MenuItemAutoUpdatingLabel<uint32_t> {
    static constexpr const char *const label = N_("Pellets");

public:
    MI_NOZZLE_CLEANER_FILL();
};

#endif

#if HAS_INDX()
/// Tune menu: fine-tune of the calibrated nozzle cleaner X position
class MI_NOZZLE_CLEANER_X_OFFSET : public WiSpin {
    static constexpr const char *const label = N_("Nozzle Cleaner X Offset");

public:
    MI_NOZZLE_CLEANER_X_OFFSET();

protected:
    virtual void OnClick() override;
};

/// Tune menu: fine-tune of the calibrated nozzle cleaner Y position
class MI_NOZZLE_CLEANER_Y_OFFSET : public WiSpin {
    static constexpr const char *const label = N_("Nozzle Cleaner Y Offset");

public:
    MI_NOZZLE_CLEANER_Y_OFFSET();

protected:
    virtual void OnClick() override;
};
#endif

class MI_MESH_BED : public IWindowMenuItem {
    static constexpr const char *const label = N_("Mesh Bed Leveling");

public:
    MI_MESH_BED();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_DISABLE_MOTORS : public IWindowMenuItem {
public:
    MI_DISABLE_MOTORS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SAVE_DUMP : public IWindowMenuItem {
    static constexpr const char *const label = N_("Save Crash Dump");

public:
    MI_SAVE_DUMP();

protected:
    virtual void click(IWindowMenu &window_menu) override;
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

class MI_INFO_FILAMENT_SENSOR : public MenuItemAutoUpdatingLabel<std::optional<FilamentSensorStateAndValue>> {
public:
    MI_INFO_FILAMENT_SENSOR(const string_view_utf8 &label, const GetterFunction &getter_function);

    void print_val(const std::span<char> &buffer) const;
    static std::optional<FilamentSensorStateAndValue> get_value(IFSensor *fsensor);
};

#if HAS_EXTRUDER_FSENSOR()
class MI_INFO_EXTRUDER_FILAMENT_SENSOR : public MI_INFO_FILAMENT_SENSOR {
public:
    MI_INFO_EXTRUDER_FILAMENT_SENSOR(std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool = CurrentlySelectedTool {});

private:
    std::optional<FilamentSensorStateAndValue> value() const;

    const std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool_;
    StringViewUtf8Parameters<4> label_params_;
};
#endif // HAS_EXTRUDER_FSENSOR()

class MI_INFO_SIDE_FILAMENT_SENSOR : public MI_INFO_FILAMENT_SENSOR {
public:
    MI_INFO_SIDE_FILAMENT_SENSOR(std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool = CurrentlySelectedTool {});

private:
    std::optional<FilamentSensorStateAndValue> value() const;

    const std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool_;
    StringViewUtf8Parameters<4> label_params_;
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

#if BOARD_IS_XBUDDY()

class MI_INFO_BED_VOLTAGE : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_BED_VOLTAGE();
};

    #if !HAS_INDX() // INDX head circumvents this
class MI_INFO_HEATER_VOLTAGE : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_HEATER_VOLTAGE();
};

class MI_INFO_HEATER_CURRENT : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_HEATER_CURRENT();
};
    #endif

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

#if HAS_INDX()
class MI_INFO_INDX_PICKUP_FAIL : public MenuItemAutoUpdatingLabel<uint16_t> {
public:
    MI_INFO_INDX_PICKUP_FAIL();
};
class MI_INFO_INDX_PARK_FAIL : public MenuItemAutoUpdatingLabel<uint16_t> {
public:
    MI_INFO_INDX_PARK_FAIL();
};
#endif

class MI_FOOTER_RESET : public IWindowMenuItem {
    static constexpr const char *const label = N_("Reset");

public:
    MI_FOOTER_RESET();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
class MI_FILAMENT_CHANGE_PREHEAT_ALL : public MenuItemSwitch {
public:
    MI_FILAMENT_CHANGE_PREHEAT_ALL();

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
    constexpr static const char *const label = N_("Device Hash In QR");

public:
    MI_DEVHASH_IN_QR();
    virtual void OnChange(size_t old_index) override;
};

class MI_LOAD_SETTINGS : public IWindowMenuItem {
    constexpr static const char *const label = N_("Load Settings From File");

public:
    MI_LOAD_SETTINGS();

    virtual void click(IWindowMenu &) override;
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
class MI_SIDE_LEDS_MAX_BRIGTHNESS : public WiSpin {

    static constexpr const char *const label =
    #if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
        N_("Chamber Lights");
    #else
        N_("RGB Side Strip");
    #endif

public:
    MI_SIDE_LEDS_MAX_BRIGTHNESS();
    virtual void OnClick() override;
};

class MI_SIDE_LEDS_DIMMED_BRIGTHNESS : public WiSpin {

    static constexpr const char *const label =
    #if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
        N_("Chamber Lights Dimmed");
    #else
        N_("RGB Side Strip Dimmed");
    #endif

public:
    MI_SIDE_LEDS_DIMMED_BRIGTHNESS();
    virtual void OnClick() override;
    virtual void Loop() override;
};

class MI_SIDE_LEDS_DIMMING_ENABLE : public MenuItemSwitch {
    static constexpr const char *const label =
    #if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
        N_("Chamber Dimming");
    #else
        N_("RGB Side Strip Dimming");
    #endif

public:
    MI_SIDE_LEDS_DIMMING_ENABLE();
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
#endif

#if HAS_POWER_PANIC()
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
#endif

#if HAS_INDX()
using MI_FIX_STUCK_NOZZLE = WithConstructorArgs<MenuItemGcodeAction, N_("Release Stuck Nozzle"), "M1984"_tstr>;
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

#if HAS_AUTO_RETRACT()
class MI_PRE_NOZZLE_CLEANING_RETRACT : public WI_ICON_SWITCH_OFF_ON_t {
public:
    MI_PRE_NOZZLE_CLEANING_RETRACT();
    void OnChange(size_t) final;
};
#endif
