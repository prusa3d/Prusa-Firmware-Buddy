// client_response_texts.cpp
// texts for all types of response any Dialog can return

#include "client_response_texts.hpp"
#include "i18n.h"
#include "png_resources.hpp"

// clang-format off
/**
 * @brief list of all button types
 * order and count must match to enum class Response in general_response.hpp!
 * TODO make some automatic checks names vs enum
 */
const std::array<BtnResource, static_cast<size_t>(Response::_last) + 1> BtnResponse::texts_and_icons {
    std::make_pair( "",                     nullptr ),                  // _none
    std::make_pair( N_("ABORT"),            &png::disconnect_48x48 ),   // Abort
    std::make_pair( N_("ABORT"),            &png::disconnect_48x48 ),   // Abort_invalidate_test
    std::make_pair( "ABS",                  &png::spool_58x58 ),        // ABS filament, do not translate
    std::make_pair( N_("Adjust"),           &png::settings_58x58 ),     // Adjust
    std::make_pair( N_("ALL"),              nullptr ),                  // All
    std::make_pair( "ASA",                  &png::spool_58x58 ),        // ASA filament, do not translate
    std::make_pair( N_("BACK"),             &png::back_32x32 ),         // Back
    std::make_pair( N_("CANCEL"),           &png::disconnect_48x48 ),   // Cancel
    std::make_pair( N_("CHANGE"),           nullptr ),                  // Change
    std::make_pair( N_("CONTINUE"),         &png::resume_48x48 ),       // Continue
    std::make_pair( N_("COOLDOWN"),         nullptr ),                  // Cooldown
    std::make_pair( N_("DISABLE"),          nullptr ),                  // Disable
    std::make_pair( N_("DISABLE SENSOR"),   nullptr ),                  // Filament_removed
    std::make_pair( "FLEX",                 &png::spool_58x58 ),        // FLEX filament, do not translate
    std::make_pair( N_("DISABLE FS"),       nullptr ),                  // FS_disable
    std::make_pair( N_("HIGH-FLOW"),        nullptr ),                  // HighFlow
    std::make_pair( "HIPS",                 &png::spool_58x58 ),        // HIPS filament, do not translate
    std::make_pair( N_("IGNORE"),           nullptr ),                  // Ignore
    std::make_pair( N_("LOAD"),             &png::spool_58x58 ),        // Load
    std::make_pair( N_("MENU"),             nullptr ),                  // Menu
    std::make_pair( N_("DISABLE MMU"),      &png::disconnect_48x48 ),   // MMU_disable
    std::make_pair( N_("NEVER"),            nullptr ),                  // Never
    std::make_pair( N_("NEXT"),             nullptr ),                  // Next
    std::make_pair( N_("NO"),               nullptr ),                  // No
    std::make_pair( N_("NOT NOW"),          nullptr ),                  // Not now
    std::make_pair( N_("OK"),               nullptr ),                  // Ok
    std::make_pair( N_("PAUSE"),            &png::pause_58x58 ),        // Pause
    std::make_pair( "PC",                   &png::spool_58x58 ),        // PC filament, do not translate
    std::make_pair( "PETG",                 &png::spool_58x58 ),        // PETG filament, do not translate
    std::make_pair( "PETG_NH",              &png::spool_58x58 ),        // PETG_NH
    std::make_pair( "PLA",                  &png::spool_58x58 ),        // PLA filament, do not translate
    std::make_pair( "PP",                   &png::spool_58x58 ),        // PP filament, do not translate
    std::make_pair( "Print",                &png::print_58x58 ),        // Print
    std::make_pair( N_("PRUSA STOCK"),      nullptr ),                  // PrusaStock
    std::make_pair( N_("PURGE MORE"),       &png::spool_58x58 ),        // Purge_more
    std::make_pair( "PVB",                  &png::spool_58x58 ),        // PVB filament, do not translate
    std::make_pair( N_("REHEAT"),           &png::preheat_58x58 ),      // Reheat
    std::make_pair( N_("RESTART"),          &png::restart_58x58 ),      // Restart
    std::make_pair( N_("RESUME"),           &png::resume_48x48 ),       // Resume
    std::make_pair( N_("RETRY"),            &png::back_32x32 ),         // Retry
    std::make_pair( N_("SKIP"),             &png::back_32x32 ),         // Skip
    std::make_pair( N_("SLOWLY"),           &png::spool_58x58 ),        // Slowly
    std::make_pair( N_("STOP"),             &png::stop_58x58  ),        // Stop
    std::make_pair( N_("UNLOAD"),           &png::spool_58x58 ),        // Unload
    std::make_pair( N_("YES"),              nullptr ),                  // Yes
    std::make_pair( N_("HEATUP"),           nullptr ),                  // Heatup
    std::make_pair( "PA",                   &png::spool_58x58 ),        // PA(Nylon) filament, do not translate
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
