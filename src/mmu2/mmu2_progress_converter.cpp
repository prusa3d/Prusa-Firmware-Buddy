#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_progress_converter.h"
#include "../lang/i18n.h"

namespace MMU2 {
// clang-format off
                                                       //01234567890123456789
static constexpr const char *MSG_PROGRESS_OK                = N_("OK"); ////MSG_PROGRESS_OK c=4
static constexpr const char *MSG_PROGRESS_ENGAGE_IDLER      = N_("Engaging idler"); ////MSG_PROGRESS_ENGAGE_IDLER c=20
static constexpr const char *MSG_PROGRESS_DISENGAGE_IDLER   = N_("Disengaging idler"); ////MSG_PROGRESS_DISENGAGE_IDLER c=20
static constexpr const char *MSG_PROGRESS_UNLOAD_FINDA      = N_("Unloading to FINDA"); ////MSG_PROGRESS_UNLOAD_FINDA c=20
static constexpr const char *MSG_PROGRESS_UNLOAD_PULLEY     = N_("Unloading to pulley"); ////MSG_PROGRESS_UNLOAD_PULLEY c=20
static constexpr const char *MSG_PROGRESS_FEED_FINDA       = N_("Feeding to FINDA"); ////MSG_PROGRESS_FEED_FINDA c=20
static constexpr const char *MSG_PROGRESS_FEED_EXTRUDER     = N_("Feeding to extruder"); ////MSG_PROGRESS_FEED_EXTRUDER c=20
static constexpr const char *MSG_PROGRESS_FEED_NOZZLE     = N_("Feeding to nozzle"); ////MSG_PROGRESS_FEED_NOZZLE c=20
static constexpr const char *MSG_PROGRESS_AVOID_GRIND       = N_("Avoiding grind"); ////MSG_PROGRESS_AVOID_GRIND c=20
static constexpr const char *MSG_PROGRESS_FINISHING_MOVES   = N_("Finishing moves"); ////MSG_PROGRESS_FINISHING_MOVES c=20
static constexpr const char *MSG_PROGRESS_WAIT_USER        = N_("ERR Wait for User"); ////MSG_PROGRESS_WAIT_USER c=20
static constexpr const char *MSG_PROGRESS_ERR_INTERNAL     = N_("ERR Internal"); ////MSG_PROGRESS_ERR_INTERNAL c=20
static constexpr const char *MSG_PROGRESS_ERR_HELP_FIL     = N_("ERR Help filament"); ////MSG_PROGRESS_ERR_HELP_FIL c=20
static constexpr const char *MSG_PROGRESS_ERR_TMC          = N_("ERR TMC failed"); ////MSG_PROGRESS_ERR_TMC c=20
static constexpr const char *MSG_PROGRESS_UNLOADING_FILAMENT= N_("Unloading filament"); ////MSG_PROGRESS_UNLOADING_FILAMENT c=20
static constexpr const char *MSG_PROGRESS_LOADING_FILAMENT  = N_("Loading filament"); ////MSG_PROGRESS_UNLOADING_FILAMENT c=20
static constexpr const char *MSG_PROGRESS_SELECT_SLOT       = N_("Selecting fil. slot"); ////MSG_PROGRESS_SELECT_SLOT c=20
static constexpr const char *MSG_PROGRESS_PREPARE_BLADE     = N_("Preparing blade"); ////MSG_PROGRESS_PREPARE_BLADE c=20
static constexpr const char *MSG_PROGRESS_PUSH_FILAMENT     = N_("Pushing filament"); ////MSG_PROGRESS_PUSH_FILAMENT c=20
static constexpr const char *MSG_PROGRESS_PERFORM_CUT      = N_("Performing cut"); ////MSG_PROGRESS_PERFORM_CUT c=20
static constexpr const char *MSG_PROGRESS_RETURN_SELECTOR   = N_("Returning selector"); ////MSG_PROGRESS_RETURN_SELECTOR c=20
static constexpr const char *MSG_PROGRESS_PARK_SELECTOR    = N_("Parking selector"); ////MSG_PROGRESS_PARK_SELECTOR c=20
static constexpr const char *MSG_PROGRESS_EJECT_FILAMENT    = N_("Ejecting filament"); ////MSG_PROGRESS_EJECT_FILAMENT c=20
static constexpr const char *MSG_PROGRESS_RETRACT_FINDA     = N_("Retract from FINDA"); ////MSG_PROGRESS_RETRACT_FINDA c=20
static constexpr const char *MSG_PROGRESS_HOMING          = N_("Homing"); ////MSG_PROGRESS_HOMING c=20
static constexpr const char *MSG_PROGRESS_MOVING_SELECTOR   = N_("Moving selector"); ////MSG_PROGRESS_MOVING_SELECTOR c=20
static constexpr const char *MSG_PROGRESS_FEED_FSENSOR      = N_("Feeding to FSensor"); ////MSG_PROGRESS_FEED_FSENSOR c=20
// clang-format on

static constexpr const char *progressTexts[] = {
    MSG_PROGRESS_OK,
    MSG_PROGRESS_ENGAGE_IDLER,
    MSG_PROGRESS_DISENGAGE_IDLER,
    MSG_PROGRESS_UNLOAD_FINDA,
    MSG_PROGRESS_UNLOAD_PULLEY,
    MSG_PROGRESS_FEED_FINDA,
    MSG_PROGRESS_FEED_EXTRUDER,
    MSG_PROGRESS_FEED_NOZZLE,
    MSG_PROGRESS_AVOID_GRIND,
    MSG_PROGRESS_FINISHING_MOVES,
    MSG_PROGRESS_DISENGAGE_IDLER,
    MSG_PROGRESS_ENGAGE_IDLER,
    MSG_PROGRESS_WAIT_USER,
    MSG_PROGRESS_ERR_INTERNAL,
    MSG_PROGRESS_ERR_HELP_FIL,
    MSG_PROGRESS_ERR_TMC,
    MSG_PROGRESS_UNLOADING_FILAMENT,
    MSG_PROGRESS_LOADING_FILAMENT,
    MSG_PROGRESS_SELECT_SLOT,
    MSG_PROGRESS_PREPARE_BLADE,
    MSG_PROGRESS_PUSH_FILAMENT,
    MSG_PROGRESS_PERFORM_CUT,
    MSG_PROGRESS_RETURN_SELECTOR,
    MSG_PROGRESS_PARK_SELECTOR,
    MSG_PROGRESS_EJECT_FILAMENT,
    MSG_PROGRESS_RETRACT_FINDA,
    MSG_PROGRESS_HOMING,
    MSG_PROGRESS_MOVING_SELECTOR,
    MSG_PROGRESS_FEED_FSENSOR
};

const char *ProgressCodeToText(ProgressCode pc) {
    // @@TODO ?? a better fallback option?
    return ((uint16_t)pc <= (sizeof(progressTexts) / sizeof(progressTexts[0])))
        ? progressTexts[(uint16_t)pc]
        : progressTexts[0];
}

} // namespace MMU2
