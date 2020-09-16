#include "DialogSelftestFansAxis.hpp"
#include "gui.hpp" //resource_font
#include "i18n.h"
#include "dialog_response.hpp"

/*****************************************************************************/
// clang-format off
static const char *txt_first              = N_("Finishing         \nbuffered gcodes.  \n");
//static const char *txt_parking            = N_("Parking");

static DialogSelftestFansAxis::States Factory() {
    DialogSelftestFansAxis::States ret = {
        DialogSelftestFansAxis::State { txt_first,   ClientResponses::GetResponses(PhasesSelfTest::_first),  ph_txt_none },
        DialogSelftestFansAxis::State { txt_first, ClientResponses::GetResponses(PhasesSelfTest::FansAxis), ph_txt_stop },
        DialogSelftestFansAxis::State { txt_first, ClientResponses::GetResponses(PhasesSelfTest::Cooldown), ph_txt_stop },
        DialogSelftestFansAxis::State { txt_first, ClientResponses::GetResponses(PhasesSelfTest::Heaters), ph_txt_stop },
    };

    return ret;
}
// clang-format on
/*****************************************************************************/

DialogSelftestFansAxis::DialogSelftestFansAxis(string_view_utf8 name)
    : DialogStateful<PhasesSelfTest>(name, Factory()) {
}
