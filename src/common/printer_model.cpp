#include <algorithm>

#include <option/has_mmu2.h>
#include <option/has_gcode_compatibility.h>
#include <option/signature_oak.h>

#include <common/printer_model.hpp>
#include <common/printer_model_data.hpp>
#include <common/extended_printer_type.hpp>
#include <common/printer_variant/printer_variant.hpp>

// Oak shares build-level printer version 7.1.0 with Core One to avoid bootloader changes.
// Return coreone to prevent firmware reset.
#if SIGNATURE_OAK()
// #error dead code found by automatic analyses (see BFW-5461)
static constexpr const PrinterModelInfo &firmware_base_constexpr = printer_model_info[std::to_underlying(PrinterModel::coreone)];
#else
static constexpr const PrinterModelInfo &firmware_base_constexpr = *std::find_if(printer_model_info.begin(), printer_model_info.end(), [](const PrinterModelInfo &info) {
    return info.version == PrinterVersion { PRINTER_TYPE, PRINTER_VERSION, PRINTER_SUBVERSION };
});
#endif

static_assert(firmware_base_constexpr.version.type == PRINTER_TYPE, "Mismatch printer version");

static_assert(
    std::to_underlying(PrinterModel::mk3) == 0
        && std::to_underlying(PrinterModel::mk3s) == 1
        && std::to_underlying(PrinterModel::mk3_5) == 2
        && std::to_underlying(PrinterModel::mk3_5s) == 3
        && std::to_underlying(PrinterModel::mk3_9) == 4
        && std::to_underlying(PrinterModel::mk3_9s) == 5
        && std::to_underlying(PrinterModel::mk4) == 6
        && std::to_underlying(PrinterModel::mk4s) == 7
        && std::to_underlying(PrinterModel::mini) == 8
        && std::to_underlying(PrinterModel::xl) == 9
        && std::to_underlying(PrinterModel::xl_dev_kit) == 10
        && std::to_underlying(PrinterModel::ix) == 11
        && std::to_underlying(PrinterModel::coreone) == 12
        && std::to_underlying(PrinterModel::coreonel) == 13
        && std::to_underlying(PrinterModel::coreone_indx) == 14
        && std::to_underlying(PrinterModel::coreonel_indx) == 15
        && std::to_underlying(PrinterModel::coreone_oak) == 16
        && std::to_underlying(PrinterModel::xls) == 17,
    "Order should not change, it will create a discrepancy in stored EEPROM values");

// Some checks about the printer data
static_assert([] {
    for (size_t i = 0; i < printer_model_info.size(); i++) {
        const auto &info = printer_model_info[i];

        // Check that the models are ordered correctly
        if (info.model != static_cast<PrinterModel>(i)) {
            std::abort();
        }
    }

    // Check that the printer_model_info contains all models
    if (printer_model_info.size() != static_cast<size_t>(PrinterModel::_cnt)) {
        std::abort();
    }

#if HAS_MMU2()
    // If the printer has an MMU, check that we have a record in printer_model_mmu_variant
    #if HAS_EXTENDED_PRINTER_TYPE()
    for (PrinterModel model : extended_printer_type_model) {
        if (std::ranges::find_if(printer_model_mmu_variant, [&](const auto &item) { return item.model == model; }) == printer_model_mmu_variant.end()) {
            std::abort();
        }
    }
    #else
    if (std::ranges::find_if(printer_model_mmu_variant, [](const auto &item) { return item.model == firmware_base_constexpr.model; }) == printer_model_mmu_variant.end()) {
        std::abort();
    }
    #endif
#endif

    return true;
}());

const PrinterModelInfo &PrinterModelInfo::firmware_base() {
    return firmware_base_constexpr;
}

const PrinterModelInfo &PrinterModelInfo::get(PrinterModel model) {
    return get_constexpr(model);
}

const PrinterModelInfo &PrinterModelInfo::current() {
#if SIGNATURE_OAK()
    // #error dead code found by automatic analyses (see BFW-5461)
    return get_constexpr(PrinterModel::coreone_oak);
#elif HAS_EXTENDED_PRINTER_TYPE()
    const auto model_index = config_store().extended_printer_type.get();
    if (model_index >= extended_printer_type_model.size()) {
        return firmware_base_constexpr;
    } else {
        return printer_model_info[std::to_underlying(extended_printer_type_model[model_index])];
    }
#else
    return firmware_base_constexpr;
#endif
}

const PrinterModelInfo *PrinterModelInfo::from_gcode_check_code(GCodeCheckPrinterModelCode code) {
    for (const auto &item : printer_model_mmu_variant) {
        if (item.gcode_check_code == code) {
            return &get(item.model);
        }
    }

    for (const auto &item : printer_model_info) {
        if (item.gcode_check_code == code) {
            return &item;
        }
    }

    return nullptr;
}

const PrinterModelInfo *PrinterModelInfo::from_id_str(const std::string_view &id_str) {
    for (const auto &item : printer_model_mmu_variant) {
        if (item.id_str == id_str) {
            return &get(item.model);
        }
    }

    for (const auto &item : printer_model_info) {
        if (item.id_str == id_str) {
            return &item;
        }
    }

    return nullptr;
}

constexpr PrinterGCodeCompatibilityReport gcode_compatibility_report_constexpr(const PrinterModelInfo &fw_printer, const PrinterModelInfo &gcode_printer) {
    using CompatGroup = PrinterModelCompatibilityGroup;

    PrinterGCodeCompatibilityReport result;

    if (gcode_printer.compatibility_group == fw_printer.compatibility_group) {
        result.is_compatible = true;
        return result;
    }

    // Recursively calls gcode_compatibility_report. is_compatible ends up being true if there is an upgrade path
    const auto upgrade_from = [&](PrinterModel model) {
        result = gcode_compatibility_report_constexpr(PrinterModelInfo::get_constexpr(model), gcode_printer);
    };

    switch (fw_printer.compatibility_group) {

    case CompatGroup::mini:
    case CompatGroup::xl:
    case CompatGroup::ix:
    case CompatGroup::mk3:
    case CompatGroup::coreone_indx: // INDX_TODO: Implement back compatibility
    case CompatGroup::coreonel_indx: // INDX_TODO: Implement gcode compatibility
        // No backwards compatibility
        break;

    case CompatGroup::xls:
        upgrade_from(PrinterModel::xl);
        result.xl_compatibility_mode = true;
        break;

    case CompatGroup::mk3_5:
    case CompatGroup::mk4:
        upgrade_from(PrinterModel::mk3);
        result.mk3_compatibility_mode = true;
        break;

    case CompatGroup::mk4s:
        upgrade_from(PrinterModel::mk4);
        result.mk4_compatibility_mode = true;
        break;

    case CompatGroup::coreone:
        upgrade_from(PrinterModel::mk4s);
        result.chamber_compatibility_mode = true;
        break;
    case CompatGroup::coreonel:
        upgrade_from(PrinterModel::coreone);
        // No special handling needed, we are just physically larger
        break;
    }

    // Clear all compatibility modes if we're not compatible
    if (!result.is_compatible) {
        result = {};
    }

    return result;
}

PrinterGCodeCompatibilityReport PrinterModelInfo::gcode_compatibility_report(const PrinterModelInfo &gcode_target_printer) const {
    return gcode_compatibility_report_constexpr(*this, gcode_target_printer);
}

// If printer HAS_EXTENDED_PRINTER_TYPE (i.e. XL -> XLP) it must have also HAS_GCODE_COMPATIBILITY
// otherwise the gcode sliced for master printer would generate warning message on the extended printer and vice versa.
#if HAS_EXTENDED_PRINTER_TYPE()
static_assert(HAS_GCODE_COMPATIBILITY());
#else
// In case HAS_EXTENDED_PRINTER_TYPE() is not defined, HAS_GCODE_COMPATIBILITY() should coincide with the printer being able to run MK3 gcode
// Only in case of legacy printers, for new printers the HAS_EXTENDED_PRINTER_TYPE should be used, no need to keep MK3 gcode compatibility for new printers.
static_assert(HAS_GCODE_COMPATIBILITY() == gcode_compatibility_report_constexpr(firmware_base_constexpr, PrinterModelInfo::get_constexpr(PrinterModel::mk3)).is_compatible);
#endif

// ExtendedPrinterType changes the printer's external identity (Connect printer type, USB PID, M862
// answers, error-code prefixes); PrinterVariant is a firmware-internal equipment preset that must
// never reach those surfaces. See printer_variant.hpp for the decision rule between the two.
static_assert(HAS_EXTENDED_PRINTER_TYPE() + HAS_PRINTER_VARIANT() <= 1, "At most one printer-type selection mechanism per printer");
