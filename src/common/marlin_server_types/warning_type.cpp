#include "warning_type.hpp"

PhasesWarning warning_type_phase(WarningType warning) {
    switch (warning) {

    default:
        // Intentionally returning Warning by default - only a few warnings use different phase
        return PhasesWarning::Warning;

    case WarningType::MetricsConfigChangePrompt:
        return PhasesWarning::MetricsConfigChangePrompt;

#if HAS_BED_PROBE
    case WarningType::ProbingFailed:
        return PhasesWarning::ProbingFailed;
#endif

#if HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
    case WarningType::NozzleCleaningFailed:
        return PhasesWarning::NozzleCleaningFailed;
#endif

#if HAS_UNEVEN_BED_PROMPT()
    case WarningType::BedUnevenAlignmentPrompt:
        return PhasesWarning::BedUnevenAlignmentPrompt;
#endif

#if XL_ENCLOSURE_SUPPORT()
    case WarningType::EnclosureFilterExpiration:
        return PhasesWarning::EnclosureFilterExpiration;
#endif

#if HAS_EMERGENCY_STOP()
    case WarningType::DoorOpen:
        return PhasesWarning::DoorOpen;
#endif

#if HAS_CHAMBER_API()
    case WarningType::FailedToReachChamberTemperature:
        return PhasesWarning::FailedToReachChamberTemperature;
#endif

        //
    }
}
