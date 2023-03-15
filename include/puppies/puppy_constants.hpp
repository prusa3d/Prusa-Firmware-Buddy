#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace buddy::puppies {

inline constexpr int DWARF_MAX_COUNT = 6;
inline constexpr int max_bootstrap_perc { 90 };

enum PuppyType : size_t {
    DWARF = 0,
    MODULARBED = 1,
};

/// Kennel is a location where a Puppy can live
enum class Kennel : uint8_t {
    FIRST = 0,

    MODULAR_BED = FIRST,
    DWARF_1 = 1,
    DWARF_2 = 2,
    DWARF_3 = 3,
    DWARF_4 = 4,
    DWARF_5 = 5,
    DWARF_6 = 6,
    LAST = DWARF_6,
};

constexpr const char *to_string(Kennel k) {
    switch (k) {
    case Kennel::MODULAR_BED:
        return "MODULAR_BED";
    case Kennel::DWARF_1:
        return "DWARF_1";
    case Kennel::DWARF_2:
        return "DWARF_2";
    case Kennel::DWARF_3:
        return "DWARF_3";
    case Kennel::DWARF_4:
        return "DWARF_4";
    case Kennel::DWARF_5:
        return "DWARF_5";
    case Kennel::DWARF_6:
        return "DWARF_6";
    }
    return "unspecified";
}

constexpr PuppyType to_puppy_type(Kennel kennel) {
    if (kennel == Kennel::MODULAR_BED) {
        return MODULARBED;
    } else {
        return DWARF;
    }
}

constexpr Kennel operator+(Kennel a, unsigned int b) {
    return (Kennel)(static_cast<uint8_t>(a) + static_cast<uint8_t>(b));
}

struct PuppyInfo {
    const char *name;
    const char *fw_path;
    uint8_t hw_info_hwtype; //< expected hardware info in hwtype
};

// Data about each puppy type, indexed via PuppyType enum
inline constexpr std::array<PuppyInfo, 2> puppy_info { {
    {
        "dwarf",
        "/internal/res/puppies/fw-dwarf.bin",
        42,
    },
    {
        "modularbed",
        "/internal/res/puppies/fw-modularbed.bin",
        43,
    },
} };

struct KennelInfo {
    const char *crash_dump_path; // internal path where crash dump is stored
};

/**
 * @brief Data about each kennel, indexed by the enum Kennel
 *
 */
inline constexpr auto kennel_info {
    std::to_array<KennelInfo>(
        {
            {
                "/internal/dump_modularbed.dmp",
            },
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
        })
};

constexpr uint8_t to_info_idx(Kennel kennel) {
    return static_cast<uint8_t>(kennel);
}

static_assert(static_cast<uint8_t>(Kennel::LAST) + 1 == kennel_info.size(), "Each kennel must have defined info");
}
