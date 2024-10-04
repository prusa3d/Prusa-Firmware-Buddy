#include "MItem_basic_selftest.hpp"
#include "gui.hpp"
#include "sys.h"
#include "ScreenHandler.hpp"
#include "printer_selftest.hpp"
#include <buddy/main.h>
#include "config.h"
#include <option/has_toolchanger.h>
#include "printers.h"
#include <inttypes.h>
#include <config_store/store_instance.hpp>

#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

#if HAS_TOOLCHANGER()

/*****************************************************************************/
// MI_RESTORE_CALIBRATION_FROM_USB

MI_RESTORE_CALIBRATION_FROM_USB::MI_RESTORE_CALIBRATION_FROM_USB()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::dev : is_hidden_t::yes) {
}

void MI_RESTORE_CALIBRATION_FROM_USB::click([[maybe_unused]] IWindowMenu &window_menu) {
    bool success = true;

    SelftestResult res = config_store().selftest_result.get();

    // load dock positions
    success &= prusa_toolchanger.load_tool_info_from_usb();
    prusa_toolchanger.save_tool_info();
    for (int i = 0; i < std::min<int>(config_store_ns::max_tool_count, buddy::puppies::DWARF_MAX_COUNT); i++) {
        res.tools[i].dockoffset = prusa_toolchanger.is_tool_info_valid(buddy::puppies::dwarfs[i]) ? TestResult_Passed : TestResult_Failed;
    }

    // load tool offsets
    success &= prusa_toolchanger.load_tool_offsets_from_usb();
    prusa_toolchanger.save_tool_offsets();
    for (int i = 0; i < std::min<int>(config_store_ns::max_tool_count, buddy::puppies::DWARF_MAX_COUNT); i++) {
        auto tool_offset = config_store().get_tool_offset(i);
        bool looks_fine = tool_offset.x != 0 && tool_offset.y != 0 && tool_offset.z != 0;
        res.tools[i].tooloffset = looks_fine ? TestResult_Passed : TestResult_Failed;
    }

    res = config_store().selftest_result.get();

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

        int32_t ref_nins_value = atoi(buffer.data());
        uint32_t span_value = side ? config_store_ns::defaults::side_fs_value_span : config_store_ns::defaults::extruder_fs_value_span;
        int32_t ref_ins_value = side ? config_store_ns::defaults::side_fs_ref_ins_value : config_store_ns::defaults::extruder_fs_ref_ins_value;
        ;
        char *second = strnstr(buffer.data(), " ", pos) + 1;
        if (second) {
            span_value = atoll(second);
            char *third = strnstr(second, " ", pos - (second - buffer.data())) + 1;
            if (third) {
                ref_ins_value = atoi(third);
            }
        }

        if (side) {
            config_store().set_side_fs_ref_nins_value(e, ref_nins_value);
            config_store().set_side_fs_value_span(e, span_value);
            config_store().set_side_fs_ref_ins_value(e, ref_ins_value);
        } else {
            config_store().set_extruder_fs_ref_nins_value(e, ref_nins_value);
            config_store().set_extruder_fs_value_span(e, span_value);
            config_store().set_extruder_fs_ref_ins_value(e, ref_ins_value);
        }
    }

    return fclose(file) == 0;
}

/*****************************************************************************/
// MI_BACKUP_CALIBRATION_TO_USB

MI_BACKUP_CALIBRATION_TO_USB::MI_BACKUP_CALIBRATION_TO_USB()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::dev : is_hidden_t::yes) {
}

void MI_BACKUP_CALIBRATION_TO_USB::click([[maybe_unused]] IWindowMenu &window_menu) {
    bool success = true;
    success &= prusa_toolchanger.save_tool_info_to_usb();
    success &= prusa_toolchanger.save_tool_offsets_to_file();
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

        int32_t ref_nins_value = side ? config_store().get_side_fs_ref_nins_value(e) : config_store().get_extruder_fs_ref_nins_value(e);
        uint32_t span_value = side ? config_store().get_side_fs_value_span(e) : config_store().get_extruder_fs_value_span(e);
        int32_t ref_ins_value = side ? config_store().get_side_fs_ref_ins_value(e) : config_store().get_extruder_fs_ref_ins_value(e);

        int n = snprintf(buffer.data(), buffer.size(), "%" PRIi32 " %" PRIu32 " %" PRIi32 "\n", ref_nins_value, span_value, ref_ins_value);
        fwrite(buffer.data(), sizeof(char), std::min<int>(n, buffer.size() - 1), file);
    }

    return fclose(file) == 0;
}
#endif
