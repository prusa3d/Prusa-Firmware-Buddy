#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <ranges>
#include <utility>
#include <option/has_dwarf.h>
#include <option/has_puppy_modularbed.h>
#include <option/has_xbuddy_extension.h>
#include <option/has_puppies.h>
#include <option/has_indx_head.h>
#include <option/has_xl_can.h>

namespace buddy::puppies {

static_assert(HAS_PUPPIES(), "Why do you include this file if you don't use any puppies");

inline constexpr int DWARF_MAX_COUNT = 6;
inline constexpr int max_bootstrap_perc { 90 };

enum PuppyType : size_t {
    DWARF,
    MODULARBED,
    INDX_HEAD,
};

/// Dock is a location where a Puppy can live
enum class Dock : uint8_t {
    /// Behind the XL_CAN on some pritners
    MODULAR_BED,
    DWARF_1,
    DWARF_2,
    DWARF_3,
    DWARF_4,
    DWARF_5,
    DWARF_6,

    /// Custom bootstrap procedure
    /// Between the printer and INDX_HEAD/MMU on some printers
    /// Needs to be set up before bootstrapping these
    XBUDDY_EXTENSION,

    /// Behind the XBE
    INDX_HEAD,

    /// Custom bootstrap procedure, gutted-out variant of XBE
    /// Is between the printer and MODULAR_BED on some printers
    /// Needs to be set up before bootstrapping these
    /// Unlike the XBE, the presence is optional
    XL_CAN,
};

static_assert(std::to_underlying(Dock::XBUDDY_EXTENSION) == 7, "Must stay 8th puppy, because we are unable to do dynamic address assignment on startup on xBuddy");
static_assert(std::to_underlying(Dock::INDX_HEAD) == 8, "Must stay 9th puppy, because we are unable to do dynamic address assignment on startup on xBuddy");
static_assert(std::to_underlying(Dock::XL_CAN) == 9, "Reserved as 10th dock for the XLS XL-CAN, same as XBUDDY_EXTENSION");

// DOCKS is the set of puppies driven by the standard PuppyBootstrap loop.
// Some puppies (XL-CAN, XBE) have a custom bootstrap process and are not handled here.
#if HAS_PUPPY_MODULARBED() || HAS_DWARF() || HAS_INDX_HEAD()
constexpr auto DOCKS = std::to_array({
    #if HAS_PUPPY_MODULARBED()
    Dock::MODULAR_BED,
    #endif
    #if HAS_DWARF()
        Dock::DWARF_1,
        Dock::DWARF_2,
        Dock::DWARF_3,
        Dock::DWARF_4,
        Dock::DWARF_5,
        Dock::DWARF_6,
    #endif
    #if HAS_INDX_HEAD()
        Dock::INDX_HEAD,
    #endif
});
#else
constexpr std::array<Dock, 0> DOCKS {};
#endif

using DockIterator = decltype(DOCKS)::const_iterator;

constexpr const char *to_string(Dock k) {
    switch (k) {
#if HAS_PUPPY_MODULARBED()
    case Dock::MODULAR_BED:
        return "MODULAR_BED";
#endif
#if HAS_DWARF()
    case Dock::DWARF_1:
        return "DWARF_1";
    case Dock::DWARF_2:
        return "DWARF_2";
    case Dock::DWARF_3:
        return "DWARF_3";
    case Dock::DWARF_4:
        return "DWARF_4";
    case Dock::DWARF_5:
        return "DWARF_5";
    case Dock::DWARF_6:
        return "DWARF_6";
#endif
#if HAS_INDX_HEAD()
    case Dock::INDX_HEAD:
        return "INDX_HEAD";
#endif
    default:
        std::abort();
    }
    std::unreachable();
}

constexpr PuppyType to_puppy_type(Dock dock) {
    switch (dock) {
#if HAS_PUPPY_MODULARBED()
    case Dock::MODULAR_BED:
        return MODULARBED;
#endif
#if HAS_DWARF()
    case Dock::DWARF_1:
    case Dock::DWARF_2:
    case Dock::DWARF_3:
    case Dock::DWARF_4:
    case Dock::DWARF_5:
    case Dock::DWARF_6:
        return DWARF;
#endif
#if HAS_INDX_HEAD()
    case Dock::INDX_HEAD:
        return INDX_HEAD;
#endif
    default:
        std::abort();
    }
    std::unreachable();
}

constexpr bool is_dynamicly_addressable(PuppyType puppy) {
    switch (puppy) {
#if HAS_PUPPY_MODULARBED()
    case MODULARBED:
        return true;
#endif
#if HAS_DWARF()
    case DWARF:
        return true;
#endif
#if HAS_INDX_HEAD()
    case INDX_HEAD:
        return false;
#endif
    default:
        std::abort();
    }
    std::unreachable();
}

#if HAS_DWARF()
static auto DWARFS = DOCKS | std::views::filter([](const auto dock) { return to_puppy_type(dock) == DWARF; });

constexpr size_t to_dwarf_index(Dock dock) {
    switch (dock) {
    case Dock::DWARF_1:
    case Dock::DWARF_2:
    case Dock::DWARF_3:
    case Dock::DWARF_4:
    case Dock::DWARF_5:
    case Dock::DWARF_6:
        return std::to_underlying(dock) - std::to_underlying(Dock::DWARF_1);
    default:
        std::abort();
    }
    std::unreachable();
}
#endif

struct PuppyInfo {
    const char *name;
    const char *fw_path;
    uint8_t hw_info_hwtype; //< expected hardware info in hwtype
};

// Data about each puppy type, indexed via PuppyType enum
inline constexpr PuppyInfo get_puppy_info(PuppyType puppy) {
    switch (puppy) {
#if HAS_DWARF()
    case DWARF:
        return {
            "dwarf",
            "/internal/res/puppies/fw-dwarf.bin",
            42,
        };
#endif
#if HAS_PUPPY_MODULARBED()
    case MODULARBED:
        return {
            "modularbed",
            "/internal/res/puppies/fw-modularbed.bin",
            43,
        };
#endif
#if HAS_INDX_HEAD()
    case INDX_HEAD:
        return {
            "indx_head",
            "/internal/res/puppies/fw-indx_head.bin",
            45,
        };
#endif
    default:
        std::abort();
    }
    std::unreachable();
}

/// Get path on filesystem associated with given dock.
inline constexpr std::optional<const char *> get_crash_dump_path(Dock dock) {
    switch (dock) {
#if HAS_PUPPY_MODULARBED()
    case Dock::MODULAR_BED:
        return "/internal/dump_modularbed.dmp";
#endif
#if HAS_DWARF()
    case Dock::DWARF_1:
        return "/internal/dump_dwarf1.dmp";
    case Dock::DWARF_2:
        return "/internal/dump_dwarf2.dmp";
    case Dock::DWARF_3:
        return "/internal/dump_dwarf3.dmp";
    case Dock::DWARF_4:
        return "/internal/dump_dwarf4.dmp";
    case Dock::DWARF_5:
        return "/internal/dump_dwarf5.dmp";
    case Dock::DWARF_6:
        return "/internal/dump_dwarf6.dmp";
#endif
#if HAS_INDX_HEAD()
    case Dock::INDX_HEAD:
        return {};
#endif
    default:
        std::abort();
    }
    std::unreachable();
}

} // namespace buddy::puppies
