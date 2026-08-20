#include "PrusaGcodeSuite.hpp"

#include <gcode/gcode.h>
#include <logging/log.hpp>

#include <algorithm>
#include <utility>

#include <option/has_auto_retract.h>
#if HAS_AUTO_RETRACT()
    #include <feature/auto_retract/auto_retract.hpp>
#endif

#include <option/has_indx.h>
#include <option/has_wastebin_fill_tracking.h>
#if HAS_WASTEBIN_FILL_TRACKING()
    #include <feature/wastebin_watcher/wastebin_watcher.hpp>
#endif
#include <option/has_nozzle_cleaner.h>
#include <option/has_nozzle_cleaner_lite.h>
#if HAS_NOZZLE_CLEANER()
    #include <nozzle_cleaner.hpp>
#elif HAS_NOZZLE_CLEANER_LITE()
    #include <nozzle_cleaner_lite.hpp>
#else
    #error "G12 requires a nozzle cleaner"
#endif

#include <printers.h>
#include <marlin_server.hpp>

LOG_COMPONENT_REF(PRUSA_GCODE);

/** \addtogroup G-Codes
 * @{
 */

/**
 * ### G12: Clean nozzle on Nozzle Cleaner <a href="https://reprap.org/wiki/G-code#G12:_Clean_Tool">G12: Clean Tool</a>
 *
 * #### Usage
 *
 *     G12 [ R ] [ S ]
 *
 * #### Parameters
 *
 * - `R` - Ensure filament is (auto-)retracted before cleaning
 * - `S` - (INDX only) Cleaning sequence number (default: 0 = clean)
 *         0 = clean, 1 = quick_clean, 2 = deep_clean, 20 = purge_clean,
 *         21 = power_panic_purge, 30 = eject_blob, 90 = enter_cleaner, 91 = exit_cleaner
 *
 */

void PrusaGcodeSuite::G12() {
    GCodeParser2 parser;
    if (!parser.parse_marlin_command()) {
        return;
    }

#if HAS_AUTO_RETRACT()
    bool auto_retract = false;
    (void)parser.store_option_if_present('R', auto_retract);

    if (auto_retract) {
        buddy::auto_retract().maybe_retract_from_nozzle();
    }
#endif

#if HAS_NOZZLE_CLEANER_LITE()
    if (nozzle_cleaner_lite::is_available()) {
        // Unlike G29, G12 doesn't already have a print_status_message FSM open,
        // so open one here to show the heating wait screen during cleaning.
        marlin_server::FSM_Holder fsm_holder(PhaseWait::print_status_message);
        if (!nozzle_cleaner_lite::clean(nozzle_cleaner_lite::CleanType::standalone)) {
            SERIAL_ERROR_MSG("G12: nozzle cleaning failed");
        }
    }
#else
    {
        static constexpr std::pair<uint16_t, nozzle_cleaner::Sequence> s_param_map[] = {
            { 0, nozzle_cleaner::Sequence::clean },
    #if HAS_INDX()
            { 1, nozzle_cleaner::Sequence::quick_clean },
            { 2, nozzle_cleaner::Sequence::deep_clean },
    #endif
            { 20, nozzle_cleaner::Sequence::purge_clean },
    #if HAS_INDX()
            { 21, nozzle_cleaner::Sequence::power_panic_purge },
            // Reserved for more purge sequences
            { 30, nozzle_cleaner::Sequence::eject_blob },
            // Reserved for other sequences
            { 90, nozzle_cleaner::Sequence::enter_cleaner },
            { 91, nozzle_cleaner::Sequence::exit_cleaner },
    #endif
        };
        static_assert(std::size(s_param_map) == nozzle_cleaner::externally_invocable_count);

        uint16_t s_param = 0;
        (void)parser.store_option_if_present('S', s_param);

        const auto it = std::ranges::find(s_param_map, s_param, &std::pair<uint16_t, nozzle_cleaner::Sequence>::first);
        if (it == std::end(s_param_map)) {
            log_warning(PRUSA_GCODE, "G12 S%u: unknown sequence", s_param);
            return;
        }

        [[maybe_unused]] const bool executed = nozzle_cleaner::load_and_execute(it->second);
    #if HAS_WASTEBIN_FILL_TRACKING()
        if (executed && it->second == nozzle_cleaner::Sequence::eject_blob) {
            // One ejected blob == one pellet; WastebinWatcher handles the mid-print full detection.
            //
            // KNOWN LIMITATION (BFW-8884): this only counts blobs ejected via G12 S30,
            // i.e. the slicer's per-toolchange ejects. Other sequences that deposit material into the
            // wastebin are NOT accounted for: purge_clean / power_panic_purge (each extrudes ~25 mm)
            // and the eject_blob + purge_clean + deep_clean run during tool-offset calibration / tool
            // prep, which call nozzle_cleaner::load_and_execute() directly (bypassing G12). As a
            // result a printer doing only single-material prints can slowly fill the bin without ever
            // tripping the warning. A proper fix would move the accounting down into
            // nozzle_cleaner::load_and_execute() (where all paths converge) and weight purges by their
            // larger deposit; tracked separately.
            WastebinWatcher::instance().account_ejected_pellet();
        }
    #endif
    }
#endif
}

/** @}*/
