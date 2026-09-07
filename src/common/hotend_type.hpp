#pragma once

#include <stdint.h>

#include <i18n.h>

#include <printers.h>

#include <option/has_hotend_type_support.h>
#if !HAS_HOTEND_TYPE_SUPPORT()
    #error This header should only be included if HAS_HOTEND_TYPE_SUPPORT()
#endif

#include <option/has_ht_hotend.h>

/// Shared for all printers.
/// !!! Never change order, never remove items - this is used in config store
enum class HotendType : uint8_t {
    stock = 0,

#if !PRINTER_IS_PRUSA_MINI()
    /// Stock Prusa hotend with sillicone sock
    stock_with_sock = 1,
#endif

#if PRINTER_IS_PRUSA_MK3_5()
    /// E3D Revo (MK3.5 only)
    e3d_revo = 2,
#endif

#if HAS_HT_HOTEND()
    /// High-temperature hotend (PT1000). Auto-detected at boot, never user-selected — so it
    /// is deliberately absent from hotend_type_list below. Sock-incompatible: its heat
    /// emission is set by a nickel-plated surface that cannot take a silicone sock.
    high_temp = 3,
#endif
};

const char *hotend_type_name(HotendType t);

/// Hotend types alter the parameters of the heater selftest - this number describes how.
int8_t hotend_type_heater_selftest_offset(HotendType t);

/// Filtered and ordered list of hotend types, for UI purposes
static constexpr std::array hotend_type_list {
    HotendType::stock,
#if !PRINTER_IS_PRUSA_MINI()
        HotendType::stock_with_sock,
#endif
#if PRINTER_IS_PRUSA_MK3_5()
        HotendType::e3d_revo,
#endif
};

/// Whether only the "stock" and "sock" options are supported
/// This affects some texts and dialogs:
/// true -> "Do you have nozzle sock installed?"
/// false -> "What hotend do you have?"
static constexpr bool hotend_type_only_sock = (hotend_type_list.size() == 2 && hotend_type_list[0] == HotendType::stock && hotend_type_list[1] == HotendType::stock_with_sock);
