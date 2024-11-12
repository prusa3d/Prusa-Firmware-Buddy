#pragma once

#include <utility>
#include <array>

#include "printer_model.hpp"

inline constexpr std::array printer_model_info {
    PrinterModelInfo {
        .model = PrinterModel::mk3,
        .compatibility_group = PrinterModelCompatibilityGroup::mk3,
        .version = { 1, 3, 0 },
        .help_url = nullptr,
        .usb_pid = 0,
        .gcode_check_code = 300,
        .id_str = "MK3",
    },
    PrinterModelInfo {
        .model = PrinterModel::mk3s,
        .compatibility_group = PrinterModelCompatibilityGroup::mk3,
        .version = { 1, 3, 1 },
        .help_url = nullptr,
        .usb_pid = 0,
        .gcode_check_code = 302,
        .id_str = "MK3S",

    },
    PrinterModelInfo {
        .model = PrinterModel::mk3_5,
        .compatibility_group = PrinterModelCompatibilityGroup::mk3_5,
        .version = { 1, 3, 5 },
        .help_url = "mk35",
        .usb_pid = 23,
        .gcode_check_code = 230,
        .id_str = "MK3.5",
    },
    PrinterModelInfo {
        .model = PrinterModel::mk3_5s,
        .compatibility_group = PrinterModelCompatibilityGroup::mk3_5,
        .version = { 1, 3, 6 },
        .help_url = "mk35s",
        .usb_pid = 28,
        .gcode_check_code = 280,
        .id_str = "MK3.5S",
    },
    PrinterModelInfo {
        .model = PrinterModel::mk3_9,
        .compatibility_group = PrinterModelCompatibilityGroup::mk4,
        .version = { 1, 3, 9 },
        .help_url = "mk39",
        .usb_pid = 21,
        .gcode_check_code = 210,
        .id_str = "MK3.9",
    },
    PrinterModelInfo {
        .model = PrinterModel::mk3_9s,
        .compatibility_group = PrinterModelCompatibilityGroup::mk4s,
        .version = { 1, 3, 10 },
        .help_url = "mk39s",
        .usb_pid = 27,
        .gcode_check_code = 270,
        .id_str = "MK3.9S",
    },
    PrinterModelInfo {
        .model = PrinterModel::mk4,
        .compatibility_group = PrinterModelCompatibilityGroup::mk4,
        .version = { 1, 4, 0 },
        .help_url = "mk4",
        .usb_pid = 13,
        .gcode_check_code = 130,
        .id_str = "MK4",
    },
    PrinterModelInfo {
        .model = PrinterModel::mk4s,
        .compatibility_group = PrinterModelCompatibilityGroup::mk4s,
        .version = { 1, 4, 1 },
        .help_url = "mk4s",
        .usb_pid = 26,
        .gcode_check_code = 260,
        .id_str = "MK4S",
    },
    PrinterModelInfo {
        .model = PrinterModel::mini,
        .compatibility_group = PrinterModelCompatibilityGroup::mini,
        .version = { 2, 1, 0 },
        .help_url = "mini",
        .usb_pid = 12,
        .gcode_check_code = 120,
        .id_str = "MINI",
    },
    PrinterModelInfo {
        .model = PrinterModel::xl,
        .compatibility_group = PrinterModelCompatibilityGroup::xl,
        .version = { 3, 1, 0 },
        .help_url = "xl",
        .usb_pid = 17,
        .gcode_check_code = 170,
        .id_str = "XL",
    },
    PrinterModelInfo {
        // XL dev kit works like standard XL
        .model = PrinterModel::xl_dev_kit,
        .compatibility_group = PrinterModelCompatibilityGroup::xl,
        .version = { 5, 1, 0 },
        .help_url = "xl",
        .usb_pid = 17,
        .gcode_check_code = 170,
        .id_str = "XL",
    },
    PrinterModelInfo {
        .model = PrinterModel::ix,
        .compatibility_group = PrinterModelCompatibilityGroup::ix,
        .version = { 4, 1, 0 },
        .help_url = "ix",
        .usb_pid = 16,
        .gcode_check_code = 160,
        .id_str = "iX",
    },
    PrinterModelInfo {
        .model = PrinterModel::cube,
        .compatibility_group = PrinterModelCompatibilityGroup::cube,
        .version = { 7, 1, 0 },
        .help_url = "core-one",
        .usb_pid = 31,
        .gcode_check_code = 310,
        .id_str = "COREONE",
    },
};

inline constexpr std::array printer_model_mmu_variant {
    PrinterModelMMUVariant {
        .model = PrinterModel::mk3,
        .gcode_check_code = 20300,
        .id_str = "MK3MMU2",
    },
    PrinterModelMMUVariant {
        .model = PrinterModel::mk3s,
        .gcode_check_code = 20302,
        .id_str = "MK3SMMU2S",
    },
    PrinterModelMMUVariant {
        .model = PrinterModel::mk3_5,
        .gcode_check_code = 30230,
        .id_str = "MK3.5MMU3",
    },
    PrinterModelMMUVariant {
        .model = PrinterModel::mk3_5s,
        .gcode_check_code = 30280,
        .id_str = "MK3.5SMMU3",
    },
    PrinterModelMMUVariant {
        .model = PrinterModel::mk3_9,
        .gcode_check_code = 30210,
        .id_str = "MK3.9MMU3",
    },
    PrinterModelMMUVariant {
        .model = PrinterModel::mk3_9s,
        .gcode_check_code = 30270,
        .id_str = "MK3.9SMMU3",
    },
    PrinterModelMMUVariant {
        .model = PrinterModel::mk4,
        .gcode_check_code = 30130,
        .id_str = "MK4MMU3",
    },
    PrinterModelMMUVariant {
        .model = PrinterModel::mk4s,
        .gcode_check_code = 30260,
        .id_str = "MK4SMMU3",
    },
    PrinterModelMMUVariant {
        .model = PrinterModel::cube,
        .gcode_check_code = 30310,
        .id_str = "COREONEMMU3",
    },
};

constexpr const PrinterModelInfo &PrinterModelInfo::get_constexpr(PrinterModel model) {
    return printer_model_info[std::to_underlying(model)];
}
