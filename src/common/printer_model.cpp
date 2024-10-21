#include <concepts>
#include <algorithm>

#include <option/has_mmu2.h>

#include <common/printer_model.hpp>
#include <common/printer_model_data.hpp>
#include <common/extended_printer_type.hpp>

static constexpr const PrinterModelInfo &firmware_base_constexpr = *std::find_if(printer_model_info.begin(), printer_model_info.end(), [](const PrinterModelInfo &info) {
    return info.version == PrinterVersion { PRINTER_TYPE, PRINTER_VERSION, PRINTER_SUBVERSION };
});

static_assert(firmware_base_constexpr.version.type == PRINTER_TYPE, "Mismatch printer version");

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
#if HAS_EXTENDED_PRINTER_TYPE()
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

PrinterGCodeCompatibilityReport PrinterModelInfo::gcode_compatibility_report(const PrinterModelInfo &gcode_target_printer) const {
    using CompatGroup = PrinterModelCompatibilityGroup;

    PrinterGCodeCompatibilityReport result;
    auto gcode_compat_group = gcode_target_printer.compatibility_group;

    // "Raise" MK3 gcode to MK4/3.5 gcode by enabling compatibility mode
    if (gcode_compat_group == CompatGroup::mk3) {
        gcode_compat_group = (this->compatibility_group == CompatGroup::mk3_5) ? CompatGroup::mk3_5 : CompatGroup::mk4;
        result.mk3_compatibility_mode = true;
    }

    // "Raise" MK4 gcode to MK4S gcode by enabling fan compatibility mode
    if (this->compatibility_group == CompatGroup::mk4s && gcode_compat_group == CompatGroup::mk4) {
        gcode_compat_group = CompatGroup::mk4s;
        result.mk4s_fan_compatibility_mode = true;
    }

    // Finally, determine if the gcode is compatible if we've reached the same compatibility group
    result.is_compatible = (this->compatibility_group == gcode_compat_group);

    return result;
}
