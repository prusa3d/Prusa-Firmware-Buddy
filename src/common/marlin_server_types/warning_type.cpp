#include "warning_type.hpp"
#include <option/has_print_sheet_detection.h>
#include <option/has_wastebin_fill_tracking.h>

#include <bitset>
#include <option/has_mmu2.h>
#include <option/has_tool_offset_sensor.h>
#include <option/has_ceiling_clearance.h>
#include <option/has_chamber_api.h>
#include <option/has_chamber_filtration_api.h>
#include <option/has_chamber_vents.h>
#include <option/has_emergency_stop.h>
#include <option/has_ht_hotend.h>
#include <option/has_human_interactions.h>
#include <option/has_precise_homing_corexy.h>
#include <option/has_uneven_bed_prompt.h>
#include <option/xl_enclosure_support.h>

constexpr PhasesWarning warning_type_phase_constexpr(WarningType warning) {
    switch (warning) {

    default:
        // Intentionally returning Warning by default - only a few warnings use different phase
        return PhasesWarning::Warning;

    case WarningType::MetricsConfigChangePrompt:
        return PhasesWarning::MetricsConfigChangePrompt;

    case WarningType::ProbingFailed:
        return PhasesWarning::ProbingFailed;

    case WarningType::FilamentSensorStuckHelp:
        return PhasesWarning::FilamentSensorStuckHelp;

#if HAS_MMU2()
    case WarningType::FilamentSensorStuckHelpMMU:
        return PhasesWarning::FilamentSensorStuckHelpMMU;
#endif

#if HAS_UNEVEN_BED_PROMPT()
    case WarningType::BedUnevenAlignmentPrompt:
        return PhasesWarning::BedUnevenAlignmentPrompt;
#endif

#if XL_ENCLOSURE_SUPPORT() || HAS_CHAMBER_FILTRATION_API()
    case WarningType::EnclosureFilterExpiration:
        return PhasesWarning::EnclosureFilterExpiration;
#endif

#if HAS_CHAMBER_VENTS()
    case WarningType::OpenChamberVents:
        return PhasesWarning::ChamberVents;
    case WarningType::CloseChamberVents:
        return PhasesWarning::ChamberVents;
#endif

#if HAS_EMERGENCY_STOP()
    case WarningType::DoorOpen:
        return PhasesWarning::DoorOpen;
#endif

#if HAS_HT_HOTEND()
    case WarningType::HotendBurnRisk:
        return PhasesWarning::HotendBurnRisk;
#endif

#if HAS_CHAMBER_API()
    case WarningType::FailedToReachChamberTemperature:
        return PhasesWarning::FailedToReachChamberTemperature;
#endif

#if HAS_PRINT_SHEET_DETECTION()
    case WarningType::SteelSheetNotDetected:
        return PhasesWarning::SteelSheetNotDetected;
#endif

#if HAS_CEILING_CLEARANCE()
    case WarningType::CeilingClearanceViolation:
        return PhasesWarning::CeilingClearanceViolation;
#endif

#if HAS_PRECISE_HOMING_COREXY()
    case WarningType::HomingCalibrationNeeded:
        return PhasesWarning::HomingCalibrationNeeded;

    case WarningType::HomingRefinementFailed:
        return PhasesWarning::HomingRefinementFailed;

    case WarningType::HomingCalibrationFromMenuNeeded:
        return PhasesWarning::HomingCalibrationFromMenuNeeded;
#endif

#if HAS_ILI9488_DISPLAY() && HAS_HUMAN_INTERACTIONS()
    case WarningType::DisplayProblemDetected:
        return PhasesWarning::DisplayProblemDetected;
#endif

#if HAS_TOOL_OFFSET_SENSOR()
    case WarningType::ToolOffsetCalibrationFailed:
        return PhasesWarning::ToolOffsetCalibrationFailed;

    case WarningType::HotendOffsetUnsafeZDeviation:
        return PhasesWarning::HotendOffsetUnsafeZDeviation;

    case WarningType::HotendOffsetUnsafeXyDeviation:
        return PhasesWarning::HotendOffsetUnsafeXyDeviation;

    case WarningType::HotendOffsetUnsafeSensorXY:
        return PhasesWarning::HotendOffsetUnsafeSensorXY;
#endif

#if HAS_WASTEBIN_FILL_TRACKING()
    case WarningType::NozzleCleanerFull:
        return PhasesWarning::NozzleCleanerFull;
    case WarningType::NozzleCleanerManualEmpty:
        return PhasesWarning::NozzleCleanerManualEmpty;
        // NozzleCleanerFullInfo intentionally has no dedicated phase - it falls back to the generic
        // PhasesWarning::Warning (single Ok), see the default below.
#endif

        //
    }
}

PhasesWarning warning_type_phase(WarningType warning) {
    return warning_type_phase_constexpr(warning);
}

constexpr uint32_t warning_lifespan_sec_constexpr(WarningType type) {
    switch (type) {

#if HAS_CHAMBER_VENTS()
    case WarningType::OpenChamberVents:
    case WarningType::CloseChamberVents:
        return 60;
#endif

    default:
        return uint32_t(-1); // Unlimited
    }
}

uint32_t warning_lifespan_sec(WarningType type) {
    return warning_lifespan_sec_constexpr(type);
}

static_assert([] {
    std::bitset<CountPhases<PhasesWarning>()> used_phases;

    // Check that each phase (except for Warning and ChamberVents, which are handled separately) has a separate phase
    // If this does not apply and we use
    // In the future, we could possibly unify WarningType and PhasesWarning
    for (size_t i = 0; i < std::to_underlying(WarningType::_cnt); i++) {
        const WarningType wt = static_cast<WarningType>(i);
        const PhasesWarning ph = warning_type_phase_constexpr(wt);
        const auto phi = std::to_underlying(ph);

        bool phase_warning_exception = ph == PhasesWarning::Warning;
#if HAS_CHAMBER_VENTS()
        phase_warning_exception |= ph == PhasesWarning::ChamberVents;
#endif

        if (!phase_warning_exception && used_phases.test(phi)) {
            std::abort();
        }

        used_phases.set(phi);
    }

    // Check that every phase is used by some warning - otherwise it's pointless
    for (size_t i = 0; i < CountPhases<PhasesWarning>(); i++) {
        if (!used_phases.test(i)) {
            std::abort();
        }
    }

    return true;
}());

#if HAS_ILI9488_DISPLAY() && HAS_HUMAN_INTERACTIONS()
// This one should be very low on the list (= priority).
// When it's popped up, it doesn't propagate to connect, so it might hide some important warnings
static_assert(std::to_underlying(WarningType::DisplayProblemDetected) == std::to_underlying(WarningType::_cnt) - 1);
#endif
