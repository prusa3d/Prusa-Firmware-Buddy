#pragma once

#include <enum_array.hpp>
#include <i18n.h>

/// Shared for all printers.
/// !!! Never change order, never remove items - this is used in config store
enum class NozzleType : uint8_t {
    Normal = 0,
    HighFlow = 1,
    _cnt,
};

static constexpr EnumArray<NozzleType, const char *, NozzleType::_cnt> nozzle_type_names {
    { NozzleType::Normal, N_("Standard") },
    { NozzleType::HighFlow, N_("High Flow") },
};

/// High-flow nozzle is not supported yet, it will be introduced later.
/// Plus it probably doesn't need a special firmware behavior.
/// So remove/hide everything related for now. Simply remove this define afterwards.
/// BFW-5171
#define NOZZLE_TYPE_SUPPORT() 0
