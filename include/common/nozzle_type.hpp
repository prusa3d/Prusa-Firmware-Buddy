#pragma once

#include <enum_array.hpp>
#include <i18n.h>
#include <printers.h>

/// For now, all nozzles have the same thermal properties for selftest.
/// Remove this when it changes.
/// BFW-5171
#define HAS_NOZZLE_TYPE_SELFETST_SUPPORT() 0

/// Shared for all printers.
/// !!! Never change order, never remove items - this is used in config store
enum class NozzleType : uint8_t {
    Normal = 0,
    HighFlow = 1,
    ObXidian = 2,
    ObXidianHF = 3,
    _cnt,
};

static constexpr EnumArray<NozzleType, const char *, NozzleType::_cnt> nozzle_type_names {
    { NozzleType::Normal, N_("Standard") },
    { NozzleType::HighFlow, N_("High Flow") },
    { NozzleType::ObXidian, N_("ObXidian") },
    { NozzleType::ObXidianHF, N_("HF ObXidian") },
};

static constexpr EnumArray<NozzleType, bool, NozzleType::_cnt> nozzle_type_enabled {
    { NozzleType::Normal, true },
    { NozzleType::HighFlow, true },
    { NozzleType::ObXidian, true },
    { NozzleType::ObXidianHF, true },
};
