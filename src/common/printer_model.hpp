#pragma once

#include <utility>

namespace printer {

/**
 * @brief Printer Model Code Enum
 *
 * @attention Printer model codes are not checked from new GCodes. We check them for compatibility reasons in old GCode files.
 *
 * For MK3 and prior the values were assigned randomly.
 * For MINI, MK4, ... and newer printers, first two numbers corespond to USB device ID and then are
 * followed by zero.
 * If the model contains mmu, then the mmu version (two numbers) are prefixed to the printer number.
 */
enum class PrinterModelCode : uint16_t {
    Invalid = 0,
    MK1 = 100,
    MK2 = 200,
    MK2MM = 201,
    MK2S = 202,
    MK2SMM = 203,
    MK2_5 = 250,
    MK2_5MMU2 = 20250,
    MK2_5S = 252,
    MK2_5SMMU2S = 20252,
    MK3 = 300,
    MK3MMU2 = 20300,
    MK3S = 302,
    MK3SMMU2S = 20302,
    MK3_5 = 230,
    MK3_5MMU3 = 30230,
    MK3_9 = 210,
    MK3_9MMU3 = 30210,
    MINI = 120,
    MK4 = 130,
    MK4MMU3 = 30130,
    MK4S = 260,
    MK4SMMU3 = 30260,
    iX = 160,
    XL = 170,
};

constexpr PrinterModelCode name_to_model_code(std::string_view name) {
    static constexpr auto name_to_model_code_table = std::to_array<std::pair<std::string_view, PrinterModelCode>>({
        { "MINI", PrinterModelCode::MINI },
        { "MK1", PrinterModelCode::MK1 },
        { "MK2", PrinterModelCode::MK2 },
        { "MK2.5", PrinterModelCode::MK2_5 },
        { "MK2.5MMU2", PrinterModelCode::MK2_5MMU2 },
        { "MK2.5S", PrinterModelCode::MK2_5S },
        { "MK2.5SMMU2S", PrinterModelCode::MK2_5SMMU2S },
        { "MK2MM", PrinterModelCode::MK2MM },
        { "MK2S", PrinterModelCode::MK2S },
        { "MK2SMM", PrinterModelCode::MK2SMM },
        { "MK3", PrinterModelCode::MK3 },
        { "MK3.5", PrinterModelCode::MK3_5 },
        { "MK3.5MMU3", PrinterModelCode::MK3_5MMU3 },
        { "MK3.9", PrinterModelCode::MK3_9 },
        { "MK3.9MMU3", PrinterModelCode::MK3_9MMU3 },
        { "MK3MMU2", PrinterModelCode::MK3MMU2 },
        { "MK3S", PrinterModelCode::MK3SMMU2S },
        { "MK3SMMU2S", PrinterModelCode::MK3SMMU2S },
        { "MK4", PrinterModelCode::MK4 },
        { "MK4MMU3", PrinterModelCode::MK4MMU3 },
        { "MK4S", PrinterModelCode::MK4S },
        { "MK4SMMU3", PrinterModelCode::MK4SMMU3 },
        { "XL", PrinterModelCode::XL },
        { "iX", PrinterModelCode::iX },
    });

    static_assert(std::ranges::is_sorted(name_to_model_code_table, std::less<std::string_view> {}, [](const auto &val) { return val.first; }));
    static_assert(name_to_model_code_table.size() == 24, "Don't forget to extend name_to_model_code_table, model_without_mmu, requires_gcode_compatibility_mode, requires_fan_compatibility_mode if needed");

    const auto found = std::ranges::lower_bound(name_to_model_code_table, name, std::less<std::string_view> {}, [](const auto &val) { return val.first; });
    if (found == name_to_model_code_table.end()) {
        return PrinterModelCode::Invalid;
    }

    return found->second;
}

constexpr PrinterModelCode model_code_without_mmu(PrinterModelCode src) {
    switch (src) {
    case PrinterModelCode::MK2_5MMU2:
        return PrinterModelCode::MK2_5;
    case PrinterModelCode::MK2_5SMMU2S:
        return PrinterModelCode::MK2_5S;
    case PrinterModelCode::MK3MMU2:
        return PrinterModelCode::MK3;
    case PrinterModelCode::MK3SMMU2S:
        return PrinterModelCode::MK3S;
    case PrinterModelCode::MK3_5MMU3:
        return PrinterModelCode::MK3_5;
    case PrinterModelCode::MK3_9MMU3:
        return PrinterModelCode::MK3_9;
    case PrinterModelCode::MK4MMU3:
        return PrinterModelCode::MK4;
    case PrinterModelCode::MK4SMMU3:
        return PrinterModelCode::MK4S;
    default:
        return src;
    }
}

#if ENABLED(GCODE_COMPATIBILITY_MK3)
constexpr bool requires_gcode_compatibility_mode(printer::PrinterModelCode code) {
    switch (code) {
    case printer::PrinterModelCode::MK3:
    case printer::PrinterModelCode::MK3S:
        return true;
    default:
        return false;
    }
}
#endif

#if ENABLED(GCODE_COMPATIBILITY_MK3)
constexpr bool requires_fan_compatibility_mode(printer::PrinterModelCode code) {
    switch (code) {
    case printer::PrinterModelCode::MK3:
    case printer::PrinterModelCode::MK3S:
    case printer::PrinterModelCode::MK3_5:
    case printer::PrinterModelCode::MK3_9:
    case printer::PrinterModelCode::MK4:
        return true;
    default:
        return false;
    }
}
#endif

} // namespace printer
