// client_response_texts.cpp
// texts for all types of response any Dialog can return

#include "client_response_texts.hpp"
#include "i18n.h"

// clang-format off
/**
 * @brief list of all button types
 * order and count must match to enum class Response in general_response.hpp!
 * TODO make some automatic checks names vs enum
 */
const std::array<std::pair<const char *, const png::Resource*>, static_cast<size_t>(Response::_last) + 1> BtnResponse::texts_and_icons {
    std::make_pair( "",                     nullptr ),                  // _none
    std::make_pair( N_("ABORT"),            png::Get<png::Id::disconnect_48x48>() ),// Abort
    std::make_pair( "ABS",                  png::Get<png::Id::spool_58x58>() ),     // ABS filament, do not translate
    std::make_pair( "ASA",                  png::Get<png::Id::spool_58x58>() ),     // ASA filament, do not translate
    std::make_pair( N_("BACK"),             png::Get<png::Id::back_32x32>() ),      // Back
    std::make_pair( N_("CANCEL"),           png::Get<png::Id::disconnect_48x48>() ),// Cancel
    std::make_pair( N_("CHANGE"),           nullptr ),                              // Change
    std::make_pair( N_("CONTINUE"),         png::Get<png::Id::resume_48x48>() ),    // Continue
    std::make_pair( N_("COOLDOWN"),         nullptr ),                              // Cooldown
    std::make_pair( N_("DISABLE SENSOR"),   nullptr ),                              // Filament_removed
    std::make_pair( "FLEX",                 png::Get<png::Id::spool_58x58>() ),     // FLEX filament, do not translate
    std::make_pair( N_("DISABLE FS"),       nullptr ),                              // FS_disable
    std::make_pair( "HIPS",                 png::Get<png::Id::spool_58x58>() ),     // HIPS filament, do not translate
    std::make_pair( N_("IGNORE"),           nullptr ),                              // Ignore
    std::make_pair( N_("LOAD"),             png::Get<png::Id::spool_58x58>() ),     // Load
    std::make_pair( N_("NEXT"),             nullptr ),                              // Next
    std::make_pair( N_("NO"),               nullptr ),                              // No
    std::make_pair( N_("OK"),               nullptr ),                              // Ok
    std::make_pair( N_("PAUSE"),            png::Get<png::Id::pause_58x58>() ),     // Pause
    std::make_pair( "PC",                   png::Get<png::Id::spool_58x58>() ),     // PC filament, do not translate
    std::make_pair( "PETG",                 png::Get<png::Id::spool_58x58>() ),     // PETG filament, do not translate
    std::make_pair( "PLA",                  png::Get<png::Id::spool_58x58>() ),     // PLA filament, do not translate
    std::make_pair( "PP",                   png::Get<png::Id::spool_58x58>() ),     // PP filament, do not translate
    std::make_pair( "Print",                png::Get<png::Id::print_58x58>() ),     // Print
    std::make_pair( N_("PURGE MORE"),       png::Get<png::Id::spool_58x58>() ),     // Purge_more
    std::make_pair( "PVB",                  png::Get<png::Id::spool_58x58>() ),     // PVB filament, do not translate
    std::make_pair( N_("REHEAT"),           png::Get<png::Id::preheat_58x58>() ),   // Reheat
    std::make_pair( N_("RESUME"),           png::Get<png::Id::resume_48x48>() ),    // Resume
    std::make_pair( N_("RETRY"),            png::Get<png::Id::back_32x32>() ),      // Retry
    std::make_pair( N_("SKIP"),             png::Get<png::Id::back_32x32>() ),      // Skip
    std::make_pair( N_("STOP"),             png::Get<png::Id::stop_58x58>()  ),     // Stop
    std::make_pair( N_("UNLOAD"),           png::Get<png::Id::spool_58x58>() ),     // Unload
    std::make_pair( N_("YES"),              nullptr )                               // Yes
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
