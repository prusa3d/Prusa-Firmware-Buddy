#pragma once

#include <cstdint>

#include <utils/enum_array.hpp>

#include <option/has_expansion_joints_gen_2.h>
#include <option/has_15gt_belts.h>
#include <option/has_chamber_vents.h>
#include <option/has_nozzle_cleaner_lite.h>
#include <option/has_indx.h>

// The editions toggle these features, so the build must actually have them.
// Nozzle cleaner lite is required only on non-INDX builds.
static_assert(HAS_EXPANSION_JOINTS_GEN_2(), "CoreOne printer_variant requires HAS_EXPANSION_JOINTS_GEN_2");
static_assert(HAS_15GT_BELTS(), "CoreOne printer_variant requires HAS_15GT_BELTS");
static_assert(HAS_CHAMBER_VENTS(), "CoreOne printer_variant requires HAS_CHAMBER_VENTS");
static_assert(HAS_INDX() || HAS_NOZZLE_CLEANER_LITE(), "non-INDX CoreOne printer_variant requires HAS_NOZZLE_CLEANER_LITE");

/// Not persisted - the shown edition is derived from the feature flags. Declaration order is the display order.
enum class PrinterVariant : uint8_t {
    base,
    plus,
    plus_gen2,
    _cnt,
};

/// Edition a factory-fresh unit currently ships as; applied on the first run and after a factory reset.
static constexpr PrinterVariant printer_variant_after_factory_reset = PrinterVariant::plus_gen2;

/// Display names (product names -> not translated).
static constexpr EnumArray<PrinterVariant, const char *, PrinterVariant::_cnt> printer_variant_names {
    { PrinterVariant::base, "CORE One" },
    { PrinterVariant::plus, "CORE One+" },
    { PrinterVariant::plus_gen2, "CORE One+ Gen2" },
};
