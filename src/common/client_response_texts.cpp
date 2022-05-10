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
const std::array<std::pair<const char *, uint16_t>, static_cast<size_t>(Response::_last) + 1> BtnResponse::texts_and_icons {
    std::make_pair( "", IDR_PNG_question_48px ),                  // _none
    std::make_pair( N_("ABORT"), IDR_PNG_disconnect_48px ),       // Abort
    std::make_pair( "ABS", IDR_PNG_spool_58px ),                  // ABS filament, do not translate
    std::make_pair( "ASA", IDR_PNG_spool_58px ),                  // ASA filament, do not translate
    std::make_pair( N_("BACK"), IDR_PNG_back_32px ),              // Back
    std::make_pair( N_("CANCEL"), IDR_PNG_disconnect_48px ),      // Cancel
    std::make_pair( N_("CHANGE"), IDR_NULL ),                     // Change
    std::make_pair( N_("CONTINUE"), IDR_PNG_resume_48px ),        // Continue
    std::make_pair( N_("COOLDOWN"), IDR_NULL ),                   // Cooldown
    std::make_pair( N_("DISABLE SENSOR"), IDR_NULL ),             // Filament_removed
    std::make_pair( "FLEX", IDR_PNG_spool_58px ),                 // FLEX filament, do not translate
    std::make_pair( "HIPS", IDR_PNG_spool_58px ),                 // HIPS filament, do not translate
    std::make_pair( N_("IGNORE"), IDR_NULL ),                     // Ignore
    std::make_pair( N_("LOAD"), IDR_PNG_spool_58px ),             // Load
    std::make_pair( N_("NEXT"), IDR_NULL ),                       // Next
    std::make_pair( N_("NO"), IDR_NULL ),                         // No
    std::make_pair( N_("OK"), IDR_NULL ),                         // Ok
    std::make_pair( "PC", IDR_PNG_spool_58px ),                   // PC filament, do not translate
    std::make_pair( "PETG", IDR_PNG_spool_58px ),                 // PETG filament, do not translate
    std::make_pair( "PLA", IDR_PNG_spool_58px ),                  // PLA filament, do not translate
    std::make_pair( "PP", IDR_PNG_spool_58px ),                   // PP filament, do not translate
    std::make_pair( N_("PURGE MORE"), IDR_PNG_spool_58px ),       // Purge_more
    std::make_pair( "PVB", IDR_PNG_spool_58px ),                  // PVB filament, do not translate
    std::make_pair( N_("REHEAT"), IDR_PNG_preheat_58px ),         // Reheat
    std::make_pair( N_("RETRY"), IDR_PNG_back_32px ),             // Retry
    std::make_pair( N_("SKIP"), IDR_PNG_back_32px ),              // Skip
    std::make_pair( N_("STOP"), IDR_PNG_stop_58px  ),             // Stop
    std::make_pair( N_("UNLOAD"), IDR_PNG_spool_58px ),           // Unload
    std::make_pair( N_("YES"), IDR_NULL )                         // Yes
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
