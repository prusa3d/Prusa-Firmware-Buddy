// client_response_texts.cpp
// texts for all types of response any Dialog can return

#include "client_response_texts.hpp"
#include "i18n.h"
#include <printers.h>

// clang-format off
/**
 * @brief list of all button types
 * order and count must match to enum class Response in general_response.hpp!
 * TODO make some automatic checks names vs enum
 */
const std::array<const char *, ftrstd::to_underlying(Response::_count)> BtnResponse::texts {
    "",                     // _none
    N_("ABORT"),            // Abort
    N_("ABORT"),            // Abort_invalidate_test
    "ABS",                  // ABS filament, do not translate
    N_("Adjust"),           // Adjust
    N_("ALL"),              // All
    "ASA",                  // ASA filament, do not translate
    N_("BACK"),             // Back
    N_("CANCEL"),           // Cancel
    N_("CHANGE"),           // Change
    N_("CONTINUE"),         // Continue
    N_("COOLDOWN"),         // Cooldown
    N_("DISABLE"),          // Disable
    N_("FILAMENT"),         // Filament
    N_("FILAMENT REMOVED"), // Filament_removed
    N_("FINISH"),           // Finish
    "FLEX",                 // FLEX filament, do not translate
    N_("DISABLE FS"),       // FS_disable
    N_("HIGH-FLOW"),        // HighFlow
    "HIPS",                 // HIPS filament, do not translate
    N_("IGNORE"),           // Ignore
    N_("LEFT"),             // Left
    N_("LOAD"),             // Load
    N_("DISABLE MMU"),      // MMU_disable
    N_("NEVER"),            // Never
    N_("NEXT"),             // Next
    N_("NO"),               // No
    N_("NOT NOW"),          // Not now
    "0.40 mm",              // NozzleDiameter_04
    "0.60 mm",              // NozzleDiameter_06
    N_("OK"),               // Ok
    N_("PAUSE"),            // Pause
    "PC",                   // PC filament, do not translate
    "PETG",                 // PETG filament, do not translate
    "PLA",                  // PLA filament, do not translate
    "PP",                   // PP filament, do not translate
    "Print",                // Print
    N_("PRUSA STOCK"),      // PrusaStock
#if PRINTER_IS_PRUSA_MINI
    N_("MORE"),             // Purge_more
#else
    N_("PURGE MORE"),       // Purge_more
#endif
    "PVB",                  // PVB filament, do not translate
    N_("QUIT"),             // QUIT
    N_("REHEAT"),           // Reheat
    N_("REPLACE"),          // Replace
    N_("REMOVE"),           // Remove
    N_("RESTART"),          // Restart
    N_("RESUME"),           // Resume
    N_("RETRY"),            // Retry
    N_("RIGHT"),            // Right
    N_("SKIP"),             // Skip
    N_("SLOWLY"),           // Slowly
    N_("SPOOL JOIN"),       // SpoolJoin
    N_("STOP"),             // Stop
    N_("UNLOAD"),           // Unload
    N_("YES"),              // Yes
    N_("HEATUP"),           // Heatup
    "PA",                   // PA(Nylon) filament, do not translate
    "PRINT",                // PRINT
};
// clang-format on

/*****************************************************************************/
// clang-format off
const PhaseTexts ph_txt_stop          = { BtnResponse::GetText(Response::Stop),     BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none) };
const PhaseTexts ph_txt_continue      = { BtnResponse::GetText(Response::Continue), BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none) };
const PhaseTexts ph_txt_continue_stop = { BtnResponse::GetText(Response::Continue), BtnResponse::GetText(Response::Stop),  BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none) };
const PhaseTexts ph_txt_none          = { BtnResponse::GetText(Response::_none),    BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none) };
const PhaseTexts ph_txt_yesno         = { BtnResponse::GetText(Response::Yes),      BtnResponse::GetText(Response::No),    BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none) };
// clang-format on
/*****************************************************************************/
