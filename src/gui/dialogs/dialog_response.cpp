// dialog_response.cpp
// texts for all types of response any Dialog can return

#include "dialog_response.hpp"
#include "i18n.h"
//todo make some automatic checks names vs enum
//list of all button types
// order and count must match to enum class Response!
const std::array<const char *, static_cast<size_t>(Response::_last) + 1> BtnTexts::texts {
    "",                   // _none
    N_("ABORT"),          // Abort
    N_("BACK"),           // Back
    N_("CANCEL"),         // Cancel
    N_("CONTINUE"),       // Continue
    N_("DISABLE SENSOR"), // Filament_removed
    N_("IGNORE"),         // Ignore
    N_("LOAD"),           // Load
    N_("NEXT"),           // Next
    N_("NO"),             // No
    N_("OK"),             // Ok
    N_("PURGE MORE"),     // Purge_more
    N_("REHEAT"),         // Reheat
    N_("RETRY"),          // Retry
    N_("STOP"),           // Stop
    N_("UNLOAD"),         // Unload
    N_("YES"),            // Yes
};

/*****************************************************************************/
// clang-format off
const PhaseTexts ph_txt_stop          = { BtnTexts::Get(Response::Stop),             BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none) };
const PhaseTexts ph_txt_continue      = { BtnTexts::Get(Response::Continue),         BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none) };
const PhaseTexts ph_txt_none          = { BtnTexts::Get(Response::_none),            BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none) };
const PhaseTexts ph_txt_yesno         = { BtnTexts::Get(Response::Yes),              BtnTexts::Get(Response::No),    BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none) };
// clang-format on
/*****************************************************************************/
