/*****************************************************************************/
// menu items running tools
#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "filament.hpp"
#include "WindowItemFormatableLabel.hpp"
#include "WindowItemFanLabel.hpp"
#include "WindowItemTempLabel.hpp"
#include "config.h"
#include <utility_extensions.hpp>

#define MI_ADDR_LEN 18

class MI_FILAMENT_SENSOR : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Filament Sensor");

    bool init_index() const;

public:
    MI_FILAMENT_SENSOR()
        : WI_ICON_SWITCH_OFF_ON_t(init_index(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void OnChange(size_t old_index) override;
};

class MI_LIVE_ADJUST_Z : public WI_LABEL_t {
    static constexpr const char *const label = N_("Live Adjust Z");

public:
    MI_LIVE_ADJUST_Z();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_AUTO_HOME : public WI_LABEL_t {
    static constexpr const char *const label = N_("Auto Home");

public:
    MI_AUTO_HOME();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MESH_BED : public WI_LABEL_t {
    static constexpr const char *const label = N_("Mesh Bed Leveling");

public:
    MI_MESH_BED();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_CALIB_Z : public WI_LABEL_t {
    static constexpr const char *const label = N_("Calibrate Z");

public:
    MI_CALIB_Z();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_DISABLE_STEP : public WI_LABEL_t {
    static constexpr const char *const label = N_("Disable Motors");

public:
    MI_DISABLE_STEP();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_FACTORY_DEFAULTS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Factory Reset");

public:
    MI_FACTORY_DEFAULTS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

#ifdef BUDDY_ENABLE_DFU_ENTRY
class MI_ENTER_DFU : public WI_LABEL_t {
    static constexpr const char *const label = "Enter DFU";

public:
    MI_ENTER_DFU();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
#endif

class MI_SAVE_DUMP : public WI_LABEL_t {
    static constexpr const char *const label = N_("Save Crash Dump");

public:
    MI_SAVE_DUMP();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_XFLASH_DELETE : public WI_LABEL_t {
    static constexpr const char *const label = "Clear External Flash"; // intentionally not translated, only for debugging

public:
    MI_XFLASH_DELETE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_XFLASH_RESET : public WI_LABEL_t {
    static constexpr const char *const label = "Delete Crash Dump"; // intentionally not translated, only for debugging

public:
    MI_XFLASH_RESET();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_HF_TEST_0 : public WI_LABEL_t {
    static constexpr const char *const label = "HF0 Test"; // intentionally not translated, only for debugging

public:
    MI_HF_TEST_0();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_HF_TEST_1 : public WI_LABEL_t {
    static constexpr const char *const label = "HF1 Test"; // intentionally not translated, only for debugging

public:
    MI_HF_TEST_1();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_LOAD_400 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.0"; // intentionally not translated, only for debugging

public:
    MI_EE_LOAD_400();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_LOAD_401 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.1"; // intentionally not translated, only for debugging

public:
    MI_EE_LOAD_401();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_LOAD_402 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.2"; // intentionally not translated, only for debugging

public:
    MI_EE_LOAD_402();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_LOAD_403RC1 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.3-RC1"; // intentionally not translated, only for debugging

public:
    MI_EE_LOAD_403RC1();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_LOAD_403 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.3"; // intentionally not translated, only for debugging

public:
    MI_EE_LOAD_403();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_LOAD : public WI_LABEL_t {
    static constexpr const char *const label = "EE Load"; // intentionally not translated, only for debugging

public:
    MI_EE_LOAD();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_SAVE : public WI_LABEL_t {
    static constexpr const char *const label = "EE Save"; // intentionally not translated, only for debugging

public:
    MI_EE_SAVE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_SAVEXML : public WI_LABEL_t {
    static constexpr const char *const label = "EE Save XML"; // intentionally not translated, only for debugging

public:
    MI_EE_SAVEXML();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_CLEAR : public WI_LABEL_t {
    static constexpr const char *const label = "EE Clear"; // intentionally not translated, only for debugging

public:
    MI_EE_CLEAR();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_M600 : public WI_LABEL_t {
    static constexpr const char *const label = N_("Change Filament");

public:
    MI_M600();

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
static constexpr size_t MI_SOUND_MODE_COUNT = 5;
#else
static constexpr size_t MI_SOUND_MODE_COUNT = 4;
#endif
class MI_SOUND_MODE : public WI_SWITCH_t<MI_SOUND_MODE_COUNT> {
    constexpr static const char *const label = N_("Sound Mode");

    constexpr static const char *str_Once = N_("Once");
    constexpr static const char *str_Loud = N_("Loud");
    constexpr static const char *str_Silent = N_("Silent");
    constexpr static const char *str_Assist = N_("Assist");
    constexpr static const char *str_Debug = "Debug";

    size_t init_index() const;

public:
    MI_SOUND_MODE();
    virtual void OnChange(size_t old_index) override;
};

class MI_SOUND_TYPE : public WI_SWITCH_t<8> {
    constexpr static const char *const label = "Sound Type";

    constexpr static const char *str_ButtonEcho = "ButtonEcho";
    constexpr static const char *str_StandardPrompt = "StandardPrompt";
    constexpr static const char *str_StandardAlert = "StandardAlert";
    constexpr static const char *str_CriticalAlert = "CriticalAlert";
    constexpr static const char *str_EncoderMove = "EncoderMove";
    constexpr static const char *str_BlindAlert = "BlindAlert";
    constexpr static const char *str_Start = "Start";
    constexpr static const char *str_SingleBeep = "SingleBeep";

public:
    MI_SOUND_TYPE();
    virtual void OnChange(size_t old_index) override;
};

class MI_SORT_FILES : public WI_SWITCH_t<2> {
    constexpr static const char *const label = N_("Sort Files");

    constexpr static const char *str_name = N_("Name");
    constexpr static const char *str_time = N_("Time");

public:
    MI_SORT_FILES();
    virtual void OnChange(size_t old_index) override;
};

class MI_SOUND_VOLUME : public WiSpinInt {
    constexpr static const char *const label = N_("Sound Volume");

public:
    MI_SOUND_VOLUME();
    virtual void OnClick() override;
    /* virtual void Change() override; */
};

class MI_TIMEZONE : public WiSpinInt {
    constexpr static const char *const label = "Time Zone Offset"; // intentionally not translated

public:
    MI_TIMEZONE();
    virtual void OnClick() override;
};

class MI_TIME_FORMAT : public WI_SWITCH_t<2> {
    constexpr static const char *const label = "Time Format"; // intentionally not translated

    constexpr static const char *str_24h = N_("24h");
    constexpr static const char *str_12h = N_("12h");

public:
    MI_TIME_FORMAT();
    virtual void OnChange(size_t old_index) override;
};

class MI_LOADCELL_SCALE : public WiSpinInt {
    constexpr static const char *const label = "Loadcell Scale";

public:
    MI_LOADCELL_SCALE();
    virtual void OnClick() override;
};

// TODO move to different files (filament sensor adc related ones ...)
class IMI_FS_SPAN : public WiSpinInt {
    eevar_id eevar;
    size_t index;

public:
    IMI_FS_SPAN(eevar_id eevar, size_t index, const char *label);
    virtual void OnClick() override;
};

template <eevar_id EEVAR_ID>
class MI_FS_SPAN : public IMI_FS_SPAN {
    static_assert(
        EEVAR_ID == EEVAR_FS_VALUE_SPAN_0 || EEVAR_ID == EEVAR_FS_VALUE_SPAN_1 || EEVAR_ID == EEVAR_FS_VALUE_SPAN_2 || EEVAR_ID == EEVAR_FS_VALUE_SPAN_3 || EEVAR_ID == EEVAR_FS_VALUE_SPAN_4 || EEVAR_ID == EEVAR_FS_VALUE_SPAN_5 || EEVAR_ID == EEVAR_SIDE_FS_VALUE_SPAN_0 || EEVAR_ID == EEVAR_SIDE_FS_VALUE_SPAN_1 || EEVAR_ID == EEVAR_SIDE_FS_VALUE_SPAN_2 || EEVAR_ID == EEVAR_SIDE_FS_VALUE_SPAN_3 || EEVAR_ID == EEVAR_SIDE_FS_VALUE_SPAN_4 || EEVAR_ID == EEVAR_SIDE_FS_VALUE_SPAN_5, "Unsupported template argument");
    struct index_data {
        const char *label;
        size_t extruder_index;
    };

    static consteval index_data get_data() {
        switch (EEVAR_ID) {
        // gui counts sensors from 1, but internally they are counted from 0
        case EEVAR_FS_VALUE_SPAN_0:
            return { N_("FS span 1"), 0 };
        case EEVAR_FS_VALUE_SPAN_1:
            return { N_("FS span 2"), 1 };
        case EEVAR_FS_VALUE_SPAN_2:
            return { N_("FS span 3"), 2 };
        case EEVAR_FS_VALUE_SPAN_3:
            return { N_("FS span 4"), 3 };
        case EEVAR_FS_VALUE_SPAN_4:
            return { N_("FS span 5"), 4 };
        case EEVAR_FS_VALUE_SPAN_5:
            return { N_("FS span 6"), 5 };
        case EEVAR_SIDE_FS_VALUE_SPAN_0:
            return { N_("Side FS span 1"), 0 };
        case EEVAR_SIDE_FS_VALUE_SPAN_1:
            return { N_("Side FS span 2"), 1 };
        case EEVAR_SIDE_FS_VALUE_SPAN_2:
            return { N_("Side FS span 3"), 2 };
        case EEVAR_SIDE_FS_VALUE_SPAN_3:
            return { N_("Side FS span 4"), 3 };
        case EEVAR_SIDE_FS_VALUE_SPAN_4:
            return { N_("Side FS span 5"), 4 };
        case EEVAR_SIDE_FS_VALUE_SPAN_5:
            return { N_("Side FS span 6"), 5 };
        default:
            consteval_assert_false();
            return { "", 0 }; // cannot happen
        }
    }

public:
    MI_FS_SPAN()
        : IMI_FS_SPAN(EEVAR_ID, get_data().extruder_index, get_data().label) {}
};

#if PRINTER_TYPE == PRINTER_PRUSA_MINI
class MI_FILAMENT_SENSOR_STATE : public WI_SWITCH_0_1_NA_t {
    static constexpr const char *const label = N_("Filament Sensor");
    static state_t get_state();

public:
    MI_FILAMENT_SENSOR_STATE();
    virtual void Loop() override;
    virtual void OnChange(size_t old_index) override {}
};

class MI_MINDA : public WI_SWITCH_0_1_NA_t {
    static constexpr const char *const label = N_("M.I.N.D.A.");
    static state_t get_state();

public:
    MI_MINDA();
    virtual void Loop() override;
    virtual void OnChange(size_t old_index) override {}
};
#endif

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
class MI_INFO_SERIAL_NUM_LOVEBOARD : public WiInfo<28> {
    static constexpr const char *const label = N_("Love Board");

public:
    MI_INFO_SERIAL_NUM_LOVEBOARD();
};
class MI_INFO_SERIAL_NUM_XLCD : public WiInfo<28> {
    static constexpr const char *const label = N_("xLCD");

public:
    MI_INFO_SERIAL_NUM_XLCD();
};

class MI_FS_AUTOLOAD : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Filament Autoloading");

public:
    MI_FS_AUTOLOAD();
    virtual void OnChange(size_t old_index) override;
};

class I_MI_INFO_HEATBREAK_N_TEMP : public WI_TEMP_LABEL_t {
    static constexpr const char *const generic_label = N_("Heatbreak Temp"); // Generic string for no toolchanger

public:
    I_MI_INFO_HEATBREAK_N_TEMP(const char *const specific_label, int index);
};

template <int N>
class MI_INFO_HEATBREAK_N_TEMP : public I_MI_INFO_HEATBREAK_N_TEMP {
    static_assert(N >= 0 && N <= 4, "bad input");
    static constexpr const char *const get_name() {
        switch (N) {
        case 0:
            return N_("Heatbreak 1 temp");
        case 1:
            return N_("Heatbreak 2 temp");
        case 2:
            return N_("Heatbreak 3 temp");
        case 3:
            return N_("Heatbreak 4 temp");
        case 4:
            return N_("Heatbreak 5 temp");
        }
    }

    static constexpr const char *const specific_label = get_name();

public:
    MI_INFO_HEATBREAK_N_TEMP()
        : I_MI_INFO_HEATBREAK_N_TEMP(specific_label, N) {
    }
};

using MI_INFO_HEATBREAK_TEMP = MI_INFO_HEATBREAK_N_TEMP<0>;

class MI_INFO_BED_TEMP : public WI_TEMP_LABEL_t {
    static constexpr const char *const label = N_("Bed Temperature");

public:
    MI_INFO_BED_TEMP();
};

class I_MI_INFO_NOZZLE_N_TEMP : public WI_TEMP_LABEL_t {
    static constexpr const char *const generic_label = N_("Nozzle Temperature"); // Generic string for no toolchanger

public:
    I_MI_INFO_NOZZLE_N_TEMP(const char *const specific_label, int index);
};

template <int N>
class MI_INFO_NOZZLE_N_TEMP : public I_MI_INFO_NOZZLE_N_TEMP {
    static_assert(N >= 0 && N <= 4, "bad input");
    static constexpr const char *const get_name() {
        switch (N) {
        case 0:
            return N_("Nozzle 1 Temperature");
        case 1:
            return N_("Nozzle 2 Temperature");
        case 2:
            return N_("Nozzle 3 Temperature");
        case 3:
            return N_("Nozzle 4 Temperature");
        case 4:
            return N_("Nozzle 5 Temperature");
        }
    }

    static constexpr const char *const specific_label = get_name();

public:
    MI_INFO_NOZZLE_N_TEMP()
        : I_MI_INFO_NOZZLE_N_TEMP(specific_label, N) {
    }
};

using MI_INFO_NOZZLE_TEMP = MI_INFO_NOZZLE_N_TEMP<0>;

class MI_INFO_LOADCELL : public WI_FORMATABLE_LABEL_t<SensorData::Value> {
    static constexpr const char *const label = N_("Loadcell Value");

public:
    MI_INFO_LOADCELL();
};

class MI_INFO_FILL_SENSOR : public WI_FORMATABLE_LABEL_t<std::pair<SensorData::Value, SensorData::Value>> {
public:
    MI_INFO_FILL_SENSOR(string_view_utf8 label);
};

class MI_INFO_PRINTER_FILL_SENSOR : public MI_INFO_FILL_SENSOR {
    static constexpr const char *label =
#if PRINTER_TYPE == PRINTER_PRUSA_XL
        N_("Tool Filament sensor");
#else
        N_("Filament Sensor");
#endif

public:
    MI_INFO_PRINTER_FILL_SENSOR();
};

class MI_INFO_SIDE_FILL_SENSOR : public MI_INFO_FILL_SENSOR {
    static constexpr const char *label = N_("Side Filament Sensor");

public:
    MI_INFO_SIDE_FILL_SENSOR();
};

class MI_INFO_PRINT_FAN : public WI_FAN_LABEL_t {
    static constexpr const char *const label = N_("Print Fan");

public:
    MI_INFO_PRINT_FAN();
};

class MI_INFO_HBR_FAN : public WI_FAN_LABEL_t {
    static constexpr const char *const label = N_("Heatbreak Fan");

public:
    MI_INFO_HBR_FAN();
};

class MI_PRINT_PROGRESS_TIME : public WiSpinInt {
    constexpr static const char *label = N_("Print Progress Screen");

public:
    MI_PRINT_PROGRESS_TIME();
    virtual void OnClick() override;
};
class MI_ODOMETER_DIST : public WI_FORMATABLE_LABEL_t<float> {
public:
    MI_ODOMETER_DIST(string_view_utf8 label, const png::Resource *icon, is_enabled_t enabled, is_hidden_t hidden, float initVal);
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
class MI_ODOMETER_DIST_E : public MI_ODOMETER_DIST {
    constexpr static const char *const label = N_("Filament");

public:
    MI_ODOMETER_DIST_E();
};
class MI_ODOMETER_TIME : public WI_FORMATABLE_LABEL_t<uint32_t> {
    constexpr static const char *const label = N_("Print Time");

public:
    MI_ODOMETER_TIME();
};

class MI_INFO_HEATER_VOLTAGE : public WI_FORMATABLE_LABEL_t<SensorData::Value> {
    static constexpr const char *const label = N_("Heater Voltage");

public:
    MI_INFO_HEATER_VOLTAGE();
};

class MI_INFO_INPUT_VOLTAGE : public WI_FORMATABLE_LABEL_t<SensorData::Value> {
    static constexpr const char *const label = N_("Input Voltage");

public:
    MI_INFO_INPUT_VOLTAGE();
};

class MI_INFO_5V_VOLTAGE : public WI_FORMATABLE_LABEL_t<SensorData::Value> {
    static constexpr const char *const label = N_("5V Voltage");

public:
    MI_INFO_5V_VOLTAGE();
};

class MI_INFO_HEATER_CURRENT : public WI_FORMATABLE_LABEL_t<SensorData::Value> {
    static constexpr const char *const label = N_("Heater Current");

public:
    MI_INFO_HEATER_CURRENT();
};

class MI_INFO_INPUT_CURRENT : public WI_FORMATABLE_LABEL_t<SensorData::Value> {
    static constexpr const char *const label = N_("Input Current");

public:
    MI_INFO_INPUT_CURRENT();
};

class MI_INFO_MMU_CURRENT : public WI_FORMATABLE_LABEL_t<SensorData::Value> {
    static constexpr const char *const label = N_("MMU Current");

public:
    MI_INFO_MMU_CURRENT();
};

class MI_INFO_SPLITTER_5V_CURRENT : public WI_FORMATABLE_LABEL_t<SensorData::Value> {
    static constexpr const char *const label = N_("Splitter 5V Current");

public:
    MI_INFO_SPLITTER_5V_CURRENT();
};

class MI_INFO_SANDWICH_5V_CURRENT : public WI_FORMATABLE_LABEL_t<SensorData::Value> {
    static constexpr const char *const label = N_("Sandwich 5V Current");

public:
    MI_INFO_SANDWICH_5V_CURRENT();
};

class MI_INFO_BUDDY_5V_CURRENT : public WI_FORMATABLE_LABEL_t<SensorData::Value> {
    static constexpr const char *const label = N_("XL Buddy 5V Current");

public:
    MI_INFO_BUDDY_5V_CURRENT();
};

class MI_INFO_BOARD_TEMP : public WI_TEMP_LABEL_t {
    static constexpr const char *const label = N_("Board Temperature");

public:
    MI_INFO_BOARD_TEMP();
};

class MI_FOOTER_RESET : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset");

public:
    MI_FOOTER_RESET();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

#if BOARD_IS_XLBUDDY
class MI_INFO_DWARF_BOARD_TEMPERATURE : public WI_TEMP_LABEL_t {
    static constexpr const char *const label = N_("Dwarf Board Temp");

public:
    MI_INFO_DWARF_BOARD_TEMPERATURE();
};

class MI_HEAT_ENTIRE_BED : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Heat Entire Bed");

    bool init_index() const;

public:
    MI_HEAT_ENTIRE_BED()
        : WI_ICON_SWITCH_OFF_ON_t(init_index(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void OnChange(size_t old_index) override;
};

class MI_INFO_MODULAR_BED_BOARD_TEMPERATURE : public WI_TEMP_LABEL_t {
    static constexpr const char *const label = N_("MBed Board Temp");

public:
    MI_INFO_MODULAR_BED_BOARD_TEMPERATURE();
};
#endif

class MI_CO_CANCEL_OBJECT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Cancel Object (Experimental)");

public:
    MI_CO_CANCEL_OBJECT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_HEATUP_BED : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Heatup Bed During Filament Operations");

public:
    MI_HEATUP_BED();

protected:
    void OnChange(size_t old_index) override;

protected:
};
