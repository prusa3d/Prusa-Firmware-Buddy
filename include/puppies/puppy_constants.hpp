#pragma once

#include <array>
#include <cstdint>
#include <ranges>
#include <utility>
#include <option/has_dwarf.h>
#include <option/has_modularbed.h>
#include <option/has_xbuddy_extension.h>
#include <option/has_puppies.h>

namespace buddy::puppies {

static_assert(HAS_PUPPIES(), "Why do you include this file if you don't use any puppies");

inline constexpr int DWARF_MAX_COUNT = 6;
inline constexpr int max_bootstrap_perc { 90 };

enum PuppyType : size_t {
    DWARF,
    MODULARBED,
    XBUDDY_EXTENSION,
};

/// Dock is a location where a Puppy can live
enum class Dock : uint8_t {
    MODULAR_BED,
    DWARF_1,
    DWARF_2,
    DWARF_3,
    DWARF_4,
    DWARF_5,
    DWARF_6,
    XBUDDY_EXTENSION,
};

static_assert(std::to_underlying(Dock::XBUDDY_EXTENSION) == 7, "Must stay 8th puppy, because we are unable to do dynamic address assignemnt on startup on xBuddy");

constexpr auto DOCKS = std::to_array({
#if HAS_MODULARBED()
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
#if HAS_XBUDDY_EXTENSION()
        Dock::XBUDDY_EXTENSION,
#endif
});

using DockIterator = decltype(DOCKS)::const_iterator;

constexpr const char *to_string(Dock k) {
    switch (k) {
#if HAS_MODULARBED()
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
#if HAS_XBUDDY_EXTENSION()
    case Dock::XBUDDY_EXTENSION:
        return "XBUDDY_EXTENSION";
#endif
    default:
        std::abort();
    }
    std::unreachable();
}

constexpr PuppyType to_puppy_type(Dock dock) {
    switch (dock) {
#if HAS_MODULARBED()
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
#if HAS_XBUDDY_EXTENSION()
    case Dock::XBUDDY_EXTENSION:
        return XBUDDY_EXTENSION;
#endif
    default:
        std::abort();
    }
    std::unreachable();
}

constexpr bool is_dynamicly_addressable(PuppyType puppy) {
    switch (puppy) {
#if HAS_MODULARBED()
    case MODULARBED:
        return true;
#endif
#if HAS_DWARF()
    case DWARF:
        return true;
#endif
#if HAS_XBUDDY_EXTENSION()
    case XBUDDY_EXTENSION:
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
#if HAS_MODULARBED()
    case MODULARBED:
        return {
            "modularbed",
            "/internal/res/puppies/fw-modularbed.bin",
            43,
        };
#endif
#if HAS_XBUDDY_EXTENSION()
    case XBUDDY_EXTENSION:
        return {
            "xbuddy extension",
            "/internal/res/puppies/fw-xbuddy-extension.bin",
            44,
        };
#endif
    default:
        std::abort();
    }
    std::unreachable();
}

struct DockInfo {
    const char *crash_dump_path; // internal path where crash dump is stored
};

/**
 * @brief Data about each dock, indexed by the enum Dock
 *
 */
inline constexpr DockInfo get_dock_info(Dock dock) {
    switch (dock) {
#if HAS_MODULARBED()
    case Dock::MODULAR_BED:
        return {
            "/internal/dump_modularbed.dmp",
        };
#endif
#if HAS_DWARF()
    case Dock::DWARF_1:
        return {
            "/internal/dump_dwarf1.dmp",
        };
    case Dock::DWARF_2:
        return {
            "/internal/dump_dwarf2.dmp",
        };
    case Dock::DWARF_3:
        return {
            "/internal/dump_dwarf3.dmp",
        };
    case Dock::DWARF_4:
        return {
            "/internal/dump_dwarf4.dmp",
        };
    case Dock::DWARF_5:
        return {
            "/internal/dump_dwarf5.dmp",
        };
    case Dock::DWARF_6:
        return {
            "/internal/dump_dwarf6.dmp",
        };
#endif
#if HAS_XBUDDY_EXTENSION()
    case Dock::XBUDDY_EXTENSION:
        return {
            "/internal/dump_xbuddy_extension.dmp",
        };
#endif
    default:
        std::abort();
    }
    std::unreachable();
}

} // namespace buddy::puppies
