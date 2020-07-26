// dialog_response.cpp
// texts for all types of response any Dialog can return

#include "dialog_response.hpp"
#include "../lang/i18n.h"
//todo make some automatic checks names vs enum
//list of all button types

const std::array<const char *, static_cast<size_t>(Response::_last) + 1> BtnTexts::texts {
    "",
    N_("YES"),
    N_("NO"),
    N_("CONTINUE"),
    N_("OK"),
    N_("BACK"),
    N_("RETRY"),
    N_("STOP"),
    N_("PURGE_MORE"),
    N_("REHEAT"),
    N_("DISABLE SENSOR")
};

const char *BtnTexts::Get(Response resp) {
    return texts[static_cast<size_t>(resp)];
}
