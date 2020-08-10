// dialog_response.cpp
// texts for all types of response any Dialog can return

#include "dialog_response.hpp"
#include "i18n.h"
//todo make some automatic checks names vs enum
//list of all button types

const std::array<const char *, static_cast<size_t>(Response::_last) + 1> BtnTexts::texts {
    "",
    N_("ABORT"),
    N_("BACK"),
    N_("CANCEL"),
    N_("CONTINUE"),
    N_("DISABLE SENSOR"),
    N_("IGNORE"),
    N_("NO"),
    N_("OK"),
    N_("PURGE MORE"),
    N_("REHEAT"),
    N_("RETRY"),
    N_("STOP"),
    N_("YES"),
};

/*****************************************************************************/
// clang-format off
const PhaseTexts ph_txt_stop          = { BtnTexts::Get(Response::Stop),             BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none) };
const PhaseTexts ph_txt_continue      = { BtnTexts::Get(Response::Continue),         BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none) };
const PhaseTexts ph_txt_none          = { BtnTexts::Get(Response::_none),            BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none) };
const PhaseTexts ph_txt_yesno         = { BtnTexts::Get(Response::Yes),              BtnTexts::Get(Response::No),    BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none) };
// clang-format on
/*****************************************************************************/
