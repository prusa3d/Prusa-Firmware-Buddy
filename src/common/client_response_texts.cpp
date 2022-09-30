// client_response_texts.cpp
// texts for all types of response any Dialog can return

#include "client_response_texts.hpp"
#include "i18n.h"
#include "resource.h"

// clang-format off
/**
 * @brief list of all button types
 * order and count must match to enum class Response in general_response.hpp!
 * TODO make some automatic checks names vs enum
 */
const std::array<std::pair<const char *, png::Id>, static_cast<size_t>(Response::_last) + 1> BtnResponse::texts_and_icons {
    std::make_pair( "", png::Id::Null() ),                  // _none
    std::make_pair( N_("ABORT"), png::Id({PNG::disconnect_48x48}) ),       // Abort
    std::make_pair( "ABS", png::Id({PNG::spool_58x58 }) ),                  // ABS filament, do not translate
    std::make_pair( "ASA", png::Id({PNG::spool_58x58 }) ),                  // ASA filament, do not translate
    std::make_pair( N_("BACK"), png::Id({PNG::back_32x32 }) ),              // Back
    std::make_pair( N_("CANCEL"), png::Id({PNG::disconnect_48x48 }) ),      // Cancel
    std::make_pair( N_("CHANGE"), png::Id::Null() ),                     // Change
    std::make_pair( N_("CONTINUE"), png::Id({PNG::resume_48x48 }) ),        // Continue
    std::make_pair( N_("COOLDOWN"), png::Id::Null() ),                   // Cooldown
    std::make_pair( N_("DISABLE SENSOR"), png::Id::Null() ),             // Filament_removed
    std::make_pair( "FLEX", png::Id({PNG::spool_58x58 }) ),                 // FLEX filament, do not translate
    std::make_pair( N_("DISABLE FS"), png::Id::Null() ),                 // FS_disable
    std::make_pair( "HIPS", png::Id({PNG::spool_58x58 }) ),                 // HIPS filament, do not translate
    std::make_pair( N_("IGNORE"), png::Id::Null() ),                     // Ignore
    std::make_pair( N_("LOAD"), png::Id({PNG::spool_58x58 }) ),             // Load
    std::make_pair( N_("NEXT"), png::Id::Null() ),                       // Next
    std::make_pair( N_("NO"), png::Id::Null() ),                         // No
    std::make_pair( N_("OK"), png::Id::Null() ),                         // Ok
    std::make_pair( N_("PAUSE"), png::Id({PNG::pause_58x58 }) ),            // Pause
    std::make_pair( "PC", png::Id({PNG::spool_58x58 }) ),                   // PC filament, do not translate
    std::make_pair( "PETG", png::Id({PNG::spool_58x58 }) ),                 // PETG filament, do not translate
    std::make_pair( "PLA", png::Id({PNG::spool_58x58 }) ),                  // PLA filament, do not translate
    std::make_pair( "PP", png::Id({PNG::spool_58x58 }) ),                   // PP filament, do not translate
    std::make_pair( "Print", png::Id({PNG::print_58x58 }) ),                // Print
    std::make_pair( N_("PURGE MORE"), png::Id({PNG::spool_58x58 }) ),       // Purge_more
    std::make_pair( "PVB", png::Id({PNG::spool_58x58 }) ),                  // PVB filament, do not translate
    std::make_pair( N_("REHEAT"), png::Id({PNG::preheat_58x58 }) ),         // Reheat
    std::make_pair( N_("RESUME"), png::Id({PNG::resume_48x48 }) ),          // Resume
    std::make_pair( N_("RETRY"), png::Id({PNG::back_32x32 }) ),             // Retry
    std::make_pair( N_("SKIP"), png::Id({PNG::back_32x32 }) ),              // Skip
    std::make_pair( N_("STOP"), png::Id({PNG::stop_58x58  }) ),             // Stop
    std::make_pair( N_("UNLOAD"), png::Id({PNG::spool_58x58 }) ),           // Unload
    std::make_pair( N_("YES"), png::Id::Null() )                        // Yes
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
