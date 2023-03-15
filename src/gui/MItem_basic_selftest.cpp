#include "MItem_basic_selftest.hpp"
#include "marlin_client.hpp"
#include "gui.hpp"
#include "sys.h"
#include "DialogHandler.hpp"
#include "ScreenHandler.hpp"
#include "printer_selftest.hpp"
#include "main.h"
#include "Pin.hpp"
#include "hwio_pindef.h"
#include "config.h"
#include "menu_spin_config.hpp"
#include "ScreenSelftest.hpp"
#include <option/has_toolchanger.h>
#include "filament_sensors_handler.hpp"
#include <inttypes.h>

#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

/*****************************************************************************/
// MI_WIZARD
MI_WIZARD::MI_WIZARD()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_WIZARD::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmWizard);
}

/*****************************************************************************/
// MI_SELFTEST
MI_SELFTEST::MI_SELFTEST()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SELFTEST::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmFullSelftest);
}

/*****************************************************************************/
// MI_SELFTEST_RESULT
MI_SELFTEST_RESULT::MI_SELFTEST_RESULT()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SELFTEST_RESULT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmShow_result);
}

/*****************************************************************************/
// MI_CALIB_FIRST
MI_CALIB_FIRST::MI_CALIB_FIRST()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, PRINTER_TYPE == PRINTER_PRUSA_MINI ? is_hidden_t::no : is_hidden_t::dev) {
}

void MI_CALIB_FIRST::click(IWindowMenu & /*window_menu*/) {
#if PRINTER_TYPE == PRINTER_PRUSA_MINI
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmFirstLayer);
#endif
}

/*****************************************************************************/
// MI_TEST_FANS
MI_TEST_FANS::MI_TEST_FANS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST_FANS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmFans);
}

/*****************************************************************************/
// MI_TEST_XYZ
MI_TEST_XYZ::MI_TEST_XYZ()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST_XYZ::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmXYZAxis);
}

/*****************************************************************************/
// MI_TEST_X
MI_TEST_X::MI_TEST_X()
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_TEST_X::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmXAxis);
}

/*****************************************************************************/
// MI_TEST_Y
MI_TEST_Y::MI_TEST_Y()
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_TEST_Y::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmYAxis);
}

/*****************************************************************************/
// MI_TEST_Z
MI_TEST_Z::MI_TEST_Z()
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_TEST_Z::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmZAxis);
}

/*****************************************************************************/
// MI_TEST_HEAT
MI_TEST_HEAT::MI_TEST_HEAT()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST_HEAT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmHeaters);
}

/*****************************************************************************/
// MI_TEST_HOTEND
MI_TEST_HOTEND::MI_TEST_HOTEND()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_TEST_HOTEND::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmHeaters_noz);
}

/*****************************************************************************/
// MI_TEST_BED
MI_TEST_BED::MI_TEST_BED()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_TEST_BED::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmHeaters_bed);
}

/*****************************************************************************/
// MI_CALIB_FSENSOR
#if FILAMENT_SENSOR_IS_ADC()
MI_CALIB_FSENSOR::MI_CALIB_FSENSOR()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_CALIB_FSENSOR::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmFSensor);
}
#endif
/*****************************************************************************/
// MI_TEST_FANS_fine
MI_ADVANCED_FAN_TEST::MI_ADVANCED_FAN_TEST()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_ADVANCED_FAN_TEST::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmFans_fine);
}

#if HAS_TOOLCHANGER()
/*****************************************************************************/
// MI_CALIBRATE_TOOL_OFFSETS
MI_CALIBRATE_TOOL_OFFSETS::MI_CALIBRATE_TOOL_OFFSETS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes) {
}

void MI_CALIBRATE_TOOL_OFFSETS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmToolOffsets);
}

/*****************************************************************************/
// MI_RESTORE_CALIBRATION_FROM_USB

MI_RESTORE_CALIBRATION_FROM_USB::MI_RESTORE_CALIBRATION_FROM_USB()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::dev : is_hidden_t::yes) {
}

void MI_RESTORE_CALIBRATION_FROM_USB::click(IWindowMenu &window_menu) {
    bool success = true;

    SelftestResult res;
    eeprom_get_selftest_results(&res);

    // load kennel positions
    success &= prusa_toolchanger.load_tool_info_from_usb();
    prusa_toolchanger.save_tool_info();
    for (int i = 0; i < std::min(EEPROM_MAX_TOOL_COUNT, DWARF_MAX_COUNT); i++) {
        res.tools[i].kenneloffset = prusa_toolchanger.is_tool_info_valid(dwarfs[i]) ? TestResult_Passed : TestResult_Failed;
    }

    // load tool offsets
    success &= prusa_toolchanger.load_tool_offsets_from_usb();
    prusa_toolchanger.save_tool_offsets();
    for (int i = 0; i < std::min(EEPROM_MAX_TOOL_COUNT, DWARF_MAX_COUNT); i++) {
        auto tool_offset = eeprom_get_tool_offset(i);
        bool looks_fine = tool_offset.x != 0 && tool_offset.y != 0 && tool_offset.z != 0;
        res.tools[i].tooloffset = looks_fine ? TestResult_Passed : TestResult_Failed;
    }

    eeprom_get_selftest_results(&res);

    // load filament sensor calibrations
    success &= restore_fs_calibration();

    if (success) {
        MsgBoxInfo(_("Calibration data restored successfully"), Responses_Ok);
    } else {
        MsgBoxError(_("Failed to restore calibration data"), Responses_Ok);
    }
}

bool MI_RESTORE_CALIBRATION_FROM_USB::restore_fs_calibration() {
    FILE *file = fopen("/usb/filamentsensors.txt", "r");

    if (file == nullptr) {
        return false;
    }

    for (int i = 0; i < EXTRUDERS * 2; i++) {
        int e = i % EXTRUDERS;
        bool side = i >= EXTRUDERS;

        std::array<char, 40> buffer;
        size_t pos = 0;

        // Read line
        while (pos < buffer.size()) {
            char c;
            if (fread(&c, 1, 1, file) != 1 || c == '\n') {
                buffer[pos++] = 0;
                break;
            }
            buffer[pos++] = c;
        }

        int32_t ref_value = atoi(buffer.data());
        char *second = strnstr(buffer.data(), " ", pos) + 1;
        uint32_t span_value = atoll(second);

        IFSensor *sensor = side ? GetSideFSensor(e) : GetExtruderFSensor(e);

        if (sensor) {
            eeprom_set_i32(sensor->get_eeprom_ref_id(), ref_value);

            eeprom_set_ui32(sensor->get_eeprom_span_id(), span_value);
        }
    }

    return fclose(file) == 0;
}

/*****************************************************************************/
// MI_BACKUP_CALIBRATION_TO_USB

MI_BACKUP_CALIBRATION_TO_USB::MI_BACKUP_CALIBRATION_TO_USB()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::dev : is_hidden_t::yes) {
}

void MI_BACKUP_CALIBRATION_TO_USB::click(IWindowMenu &window_menu) {
    bool success = true;
    success &= prusa_toolchanger.save_tool_info_to_usb();
    success &= prusa_toolchanger.save_tool_offsets_to_usb();
    success &= backup_fs_calibration();
    if (success) {
        MsgBoxInfo(_("Calibration data saved successfully"), Responses_Ok);
    } else {
        MsgBoxError(_("Failed to save calibration data"), Responses_Ok);
    }
}

bool MI_BACKUP_CALIBRATION_TO_USB::backup_fs_calibration() {
    // This is for development purposes only

    FILE *file = fopen("/usb/filamentsensors.txt", "w");

    if (file == nullptr) {
        return false;
    }

    for (int i = 0; i < EXTRUDERS * 2; i++) {
        std::array<char, 40> buffer;
        int e = i % EXTRUDERS;
        bool side = i >= EXTRUDERS;

        IFSensor *sensor = side ? GetSideFSensor(e) : GetExtruderFSensor(e);

        if (sensor) {
            int32_t ref_value = eeprom_get_i32(sensor->get_eeprom_ref_id());

            uint32_t span_value = eeprom_get_ui32(sensor->get_eeprom_span_id());

            int n = snprintf(buffer.data(), buffer.size(), "%" PRIi32 " %" PRIu32 "\n", ref_value, span_value);
            fwrite(buffer.data(), sizeof(char), n, file);
        }
    }

    return fclose(file) == 0;
}
#endif
