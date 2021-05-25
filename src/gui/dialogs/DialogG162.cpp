#include "DialogG162.hpp"
#include "gui.hpp" //resource_font
#include "i18n.h"
#include "client_response_texts.hpp"

/*****************************************************************************/
// clang-format off
static const char *txt_first              = N_("Finishing buffered gcodes.");
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

DialogG162::DialogG162(string_view_utf8 name)
    : DialogStateful<PhasesG162>(name, Factory()) {}
