#pragma once

#include <stdint.h>

#include <enum_array.hpp>
#include <i18n.h>
#include <printers.h>

/// Shared for all printers.
/// !!! Never change order, never remove items - this is used in config store
enum class HotendType : uint8_t {
    stock = 0,

    /// Stock Prusa hotend with sillicone sock
    stock_with_sock = 1,

    /// E3D Revo (MK3.5 only)
    e3d_revo = 2,

    _cnt,
};

static constexpr auto hotend_type_count = ftrstd::to_underlying(HotendType::_cnt);

static constexpr EnumArray<HotendType, const char *, HotendType::_cnt> hotend_type_names {
    { HotendType::stock, N_("Stock") },
    { HotendType::stock_with_sock, N_("With sock") },
    { HotendType::e3d_revo, N_("E3D Revo") },
};

/// Some hotend types are only supported by some printers, but the enum is the same for all -> hence this filtering array
static constexpr EnumArray<HotendType, bool, HotendType::_cnt> hotend_type_supported {
    { HotendType::stock, true },
    { HotendType::stock_with_sock, PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_iX },
    { HotendType::e3d_revo, PRINTER_IS_PRUSA_MK3_5 },
};

/// std::array<HotendType> - list of hotend types supported for this printer
static constexpr auto supported_hotend_types = [] {
    constexpr auto cnt = std::count(hotend_type_supported.begin(), hotend_type_supported.end(), true);
    std::array<HotendType, cnt> r;
    size_t j = 0;
    for (size_t i = 0; i < hotend_type_count; i++) {
        if (hotend_type_supported[i]) {
            r[j++] = static_cast<HotendType>(i);
        }
    }
    return r;
}();

/// Whether only the "stock" and "sock" options are supported
/// This affects some texts and dialogs:
/// true -> "Do you have nozzle sock installed?"
/// false -> "What hotend do you have?"
static constexpr bool hotend_type_only_sock = (supported_hotend_types.size() == 2 && hotend_type_supported[HotendType::stock] && hotend_type_supported[HotendType::stock_with_sock]);
