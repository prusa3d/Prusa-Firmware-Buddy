#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <option/has_dwarf.h>
#include <option/has_modularbed.h>

namespace buddy::puppies {

inline constexpr int DWARF_MAX_COUNT = 6;
inline constexpr int max_bootstrap_perc { 90 };

enum PuppyType : size_t {
#if HAS_DWARF()
    DWARF,
#endif
#if HAS_MODULARBED()
    MODULARBED,
#endif
    PUPPY_TYPE_NUM_ELEMENTS,
};

/// Dock is a location where a Puppy can live
enum class Dock : uint8_t {
    FIRST = 0,

#if HAS_MODULARBED()
    MODULAR_BED = FIRST,
#endif
#if HAS_DWARF()
    DWARF_1 = 1,
    DWARF_2 = 2,
    DWARF_3 = 3,
    DWARF_4 = 4,
    DWARF_5 = 5,
    DWARF_6 = 6,
    LAST = DWARF_6,
#elif HAS_MODULARBED()
    LAST = MODULAR_BED,
#else
    LAST = FIRST,
#endif
};

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
    }
    return "unspecified";
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
    default:
        return PUPPY_TYPE_NUM_ELEMENTS;
    }
}

constexpr Dock operator+(Dock a, unsigned int b) {
    return (Dock)(static_cast<uint8_t>(a) + static_cast<uint8_t>(b));
}

struct PuppyInfo {
    const char *name;
    const char *fw_path;
    uint8_t hw_info_hwtype; //< expected hardware info in hwtype
};

// Data about each puppy type, indexed via PuppyType enum
inline constexpr std::array<PuppyInfo, PUPPY_TYPE_NUM_ELEMENTS> puppy_info { {
#if HAS_DWARF()
    {
        "dwarf",
        "/internal/res/puppies/fw-dwarf.bin",
        42,
    },
#endif
#if HAS_MODULARBED()
    {
        "modularbed",
        "/internal/res/puppies/fw-modularbed.bin",
        43,
    },
#endif
} };

struct DockInfo {
    const char *crash_dump_path; // internal path where crash dump is stored
};

/**
 * @brief Data about each dock, indexed by the enum Dock
 *
 */
inline constexpr std::array<DockInfo, static_cast<size_t>(Dock::LAST) + 1> dock_info { {
#if HAS_MODULARBED()
    {
        "/internal/dump_modularbed.dmp",
    },
#endif
#if HAS_DWARF()
    {
        "/internal/dump_dwarf1.dmp",
    },
    {
        "/internal/dump_dwarf2.dmp",
    },
    {
        "/internal/dump_dwarf3.dmp",
    },
    {
        "/internal/dump_dwarf4.dmp",
    },
    {
        "/internal/dump_dwarf5.dmp",
    },
    {
        "/internal/dump_dwarf6.dmp",
    },
#endif
} };

constexpr uint8_t to_info_idx(Dock dock) {
    return static_cast<uint8_t>(dock);
}

static_assert(static_cast<uint8_t>(Dock::LAST) + 1 == dock_info.size(), "Each dock must have defined info");
} // namespace buddy::puppies
