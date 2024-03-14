// client_response_texts.cpp
// texts for all types of response any Dialog can return

#include "client_response_texts.hpp"
#include "i18n.h"
#include "img_resources.hpp"
#include <printers.h>

// clang-format off
/**
 * @brief list of all button types
 * order and count must match to enum class Response in general_response.hpp!
 * TODO make some automatic checks names vs enum
 */
const std::array<BtnResource, ftrstd::to_underlying(Response::_count)> BtnResponse::texts_and_icons {
    std::make_pair( "",                     nullptr ),                  // _none
    std::make_pair( N_("ABORT"),            &img::disconnect_48x48 ),   // Abort
    std::make_pair( N_("ABORT"),            &img::disconnect_48x48 ),   // Abort_invalidate_test
    std::make_pair( "ABS",                  &img::spool_58x58 ),        // ABS filament, do not translate
    std::make_pair( N_("Adjust"),           &img::settings_58x58 ),     // Adjust
    std::make_pair( N_("ALL"),              nullptr ),                  // All
    std::make_pair( "ASA",                  &img::spool_58x58 ),        // ASA filament, do not translate
    std::make_pair( N_("BACK"),             &img::back_32x32 ),         // Back
    std::make_pair( N_("CANCEL"),           &img::disconnect_48x48 ),   // Cancel
    std::make_pair( N_("CHANGE"),           nullptr ),                  // Change
    std::make_pair( N_("CONTINUE"),         &img::resume_48x48 ),       // Continue
    std::make_pair( N_("COOLDOWN"),         nullptr ),                  // Cooldown
    std::make_pair( N_("DISABLE"),          nullptr ),                  // Disable
    std::make_pair( N_("FILAMENT"),         nullptr ),                  // Filament
    std::make_pair( N_("FILAMENT REMOVED"), nullptr ),                  // Filament_removed
    std::make_pair( N_("FINISH"),           nullptr ),                  // Finish
    std::make_pair( "FLEX",                 &img::spool_58x58 ),        // FLEX filament, do not translate
    std::make_pair( N_("DISABLE FS"),       nullptr ),                  // FS_disable
    std::make_pair( N_("HIGH-FLOW"),        nullptr ),                  // HighFlow
    std::make_pair( "HIPS",                 &img::spool_58x58 ),        // HIPS filament, do not translate
    std::make_pair( N_("IGNORE"),           nullptr ),                  // Ignore
    std::make_pair( N_("LEFT"),             &img::arrow_left_58x58 ),   // Left
    std::make_pair( N_("LOAD"),             &img::spool_58x58 ),        // Load
    std::make_pair( N_("DISABLE MMU"),      &img::disconnect_48x48 ),   // MMU_disable
    std::make_pair( N_("NEVER"),            nullptr ),                  // Never
    std::make_pair( N_("NEXT"),             nullptr ),                  // Next
    std::make_pair( N_("NO"),               nullptr ),                  // No
    std::make_pair( N_("NOT NOW"),          nullptr ),                  // Not now
    std::make_pair( N_("OK"),               nullptr ),                  // Ok
    std::make_pair( N_("PAUSE"),            &img::pause_58x58 ),        // Pause
    std::make_pair( "PC",                   &img::spool_58x58 ),        // PC filament, do not translate
    std::make_pair( "PETG",                 &img::spool_58x58 ),        // PETG filament, do not translate
    std::make_pair( "PETG_NH",              &img::spool_58x58 ),        // PETG_NH
    std::make_pair( "PLA",                  &img::spool_58x58 ),        // PLA filament, do not translate
    std::make_pair( "PP",                   &img::spool_58x58 ),        // PP filament, do not translate
    std::make_pair( "Print",                &img::print_58x58 ),        // Print
    std::make_pair( N_("PRUSA STOCK"),      nullptr ),                  // PrusaStock
    std::make_pair(
        #if PRINTER_IS_PRUSA_MINI
         N_("MORE"),
        #else
         N_("PURGE MORE"),
         #endif
                &img::spool_58x58 ),        // Purge_more
    std::make_pair( "PVB",                  &img::spool_58x58 ),        // PVB filament, do not translate
    std::make_pair( N_("QUIT"),             nullptr ),                  // QUIT
    std::make_pair( N_("REHEAT"),           &img::preheat_58x58 ),      // Reheat
    std::make_pair( N_("REPLACE"),          nullptr),                   // Replace
    std::make_pair( N_("REMOVE"),           nullptr),                   // Remove
    std::make_pair( N_("RESTART"),          &img::restart_58x58 ),      // Restart
    std::make_pair( N_("RESUME"),           &img::resume_48x48 ),       // Resume
    std::make_pair( N_("RETRY"),            &img::back_32x32 ),         // Retry
    std::make_pair( N_("RIGHT"),            &img::arrow_right_58x58 ),  // Right
    std::make_pair( N_("SKIP"),             &img::back_32x32 ),         // Skip
    std::make_pair( N_("SLOWLY"),           &img::spool_58x58 ),        // Slowly
    std::make_pair( N_("SPOOL JOIN"),       nullptr),                   // SpoolJoin
    std::make_pair( N_("STOP"),             &img::stop_58x58  ),        // Stop
    std::make_pair( N_("UNLOAD"),           &img::spool_58x58 ),        // Unload
    std::make_pair( N_("YES"),              nullptr ),                  // Yes
    std::make_pair( N_("HEATUP"),           nullptr ),                  // Heatup
    std::make_pair( "PA",                   &img::spool_58x58 ),        // PA(Nylon) filament, do not translate
    std::make_pair( "PRINT",                nullptr ),                  // PRINT
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
