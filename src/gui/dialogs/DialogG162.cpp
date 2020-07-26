#include "DialogG162.hpp"
#include "gui.hpp" //resource_font
#include "../lang/i18n.h"
#include "dialog_response.hpp"

/*****************************************************************************/
// clang-format off

static const PhaseTexts ph_txt_stop    = { BtnTexts::Get(Response::Stop),  BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none),  BtnTexts::Get(Response::_none) };
static const PhaseTexts ph_txt_none    = { BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none),  BtnTexts::Get(Response::_none) };

static const char *txt_first              = N_("Finishing         \nbuffered gcodes.  \n");
static const char *txt_parking            = N_("Parking");

static DialogG162::States Factory() {
    DialogG162::States ret = {
        DialogG162::State { txt_first,   ClientResponses::GetResponses(PhasesG162::_first),  ph_txt_none },
        DialogG162::State { txt_parking, ClientResponses::GetResponses(PhasesG162::Parking), ph_txt_stop },
    };
    return ret;
}
// clang-format on
/*****************************************************************************/

DialogG162::DialogG162(const char *name)
    : DialogStateful<PhasesG162>(name, Factory()) {}
