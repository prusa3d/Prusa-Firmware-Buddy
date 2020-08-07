#include "DialogLoadUnload.hpp"
#include "gui.hpp" //resource_font
#include "sound.hpp"
#include "i18n.h"
#include "dialog_response.hpp"

/*****************************************************************************/
// clang-format off
static const PhaseTexts ph_txt_reheat        = { BtnTexts::Get(Response::Reheat),           BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none) };
static const PhaseTexts ph_txt_disa          = { BtnTexts::Get(Response::Filament_removed), BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none) };
static const PhaseTexts ph_txt_iscolor       = { BtnTexts::Get(Response::Yes),              BtnTexts::Get(Response::No),    BtnTexts::Get(Response::Retry), BtnTexts::Get(Response::_none) };
static const PhaseTexts ph_txt_iscolor_purge = { BtnTexts::Get(Response::Yes),              BtnTexts::Get(Response::No),    BtnTexts::Get(Response::_none), BtnTexts::Get(Response::_none) };

static const char *txt_first              = N_("Finishing         \nbuffered gcodes.  \n");
static const char *txt_parking            = N_("Parking");
static const char *txt_unparking          = N_("Unparking");
static const char *txt_wait_temp          = N_("Waiting for temp.");
static const char *txt_prep_ram           = N_("Preparing to ram");
static const char *txt_ram                = N_("Ramming");
static const char *txt_unload             = N_("Unloading");
static const char *txt_push_fil           = N_("Press CONTINUE and\npush filament into\nthe extruder.     ");
static const char *txt_make_sure_inserted = N_("Make sure the     \nfilament is       \ninserted through  \nthe sensor.       ");
static const char *txt_inserting          = N_("Inserting");
static const char *txt_is_filament_in_gear= N_("Is filament in    \nextruder gear?    ");
static const char *txt_ejecting           = N_("Ejecting");
static const char *txt_loading            = N_("Loading to nozzle");
static const char *txt_purging            = N_("Purging");
static const char *txt_is_color           = N_("Is color correct?");
static const char *txt_nozzle_cold        = N_("Nozzle is too cold.");

static DialogLoadUnload::States LoadUnloadFactory() {
    DialogLoadUnload::States ret = {
        DialogLoadUnload::State { txt_first,              ClientResponses::GetResponses(PhasesLoadUnload::_first),           ph_txt_none },
        DialogLoadUnload::State { txt_parking,            ClientResponses::GetResponses(PhasesLoadUnload::Parking),          ph_txt_stop },
        DialogLoadUnload::State { txt_wait_temp,          ClientResponses::GetResponses(PhasesLoadUnload::WaitingTemp),      ph_txt_stop },
        DialogLoadUnload::State { txt_prep_ram,           ClientResponses::GetResponses(PhasesLoadUnload::PreparingToRam),   ph_txt_stop },
        DialogLoadUnload::State { txt_ram,                ClientResponses::GetResponses(PhasesLoadUnload::Ramming),          ph_txt_stop },
        DialogLoadUnload::State { txt_unload,             ClientResponses::GetResponses(PhasesLoadUnload::Unloading),        ph_txt_stop },
        DialogLoadUnload::State { txt_unload,             ClientResponses::GetResponses(PhasesLoadUnload::RemoveFilament),   ph_txt_stop },
        DialogLoadUnload::State { txt_push_fil,           ClientResponses::GetResponses(PhasesLoadUnload::UserPush),         ph_txt_continue, DialogLoadUnload::phaseAlertSound },
        DialogLoadUnload::State { txt_nozzle_cold,        ClientResponses::GetResponses(PhasesLoadUnload::NozzleTimeout),    ph_txt_reheat },
        DialogLoadUnload::State { txt_make_sure_inserted, ClientResponses::GetResponses(PhasesLoadUnload::MakeSureInserted), ph_txt_continue, DialogLoadUnload::phaseAlertSound },
        DialogLoadUnload::State { txt_inserting,          ClientResponses::GetResponses(PhasesLoadUnload::Inserting),        ph_txt_stop },
        DialogLoadUnload::State { txt_is_filament_in_gear,ClientResponses::GetResponses(PhasesLoadUnload::IsFilamentInGear), ph_txt_yesno },
        DialogLoadUnload::State { txt_ejecting,           ClientResponses::GetResponses(PhasesLoadUnload::Ejecting),         ph_txt_none },
        DialogLoadUnload::State { txt_loading,            ClientResponses::GetResponses(PhasesLoadUnload::Loading),          ph_txt_stop },
        DialogLoadUnload::State { txt_purging,            ClientResponses::GetResponses(PhasesLoadUnload::Purging),          ph_txt_stop },
        DialogLoadUnload::State { txt_is_color,           ClientResponses::GetResponses(PhasesLoadUnload::IsColor),          ph_txt_iscolor, DialogLoadUnload::phaseAlertSound },
        DialogLoadUnload::State { txt_is_color,           ClientResponses::GetResponses(PhasesLoadUnload::IsColorPurge),     ph_txt_iscolor_purge, DialogLoadUnload::phaseAlertSound },
        DialogLoadUnload::State { txt_unparking,          ClientResponses::GetResponses(PhasesLoadUnload::Unparking),        ph_txt_stop },
    };
    return ret;
}
// clang-format on
/*****************************************************************************/

DialogLoadUnload::DialogLoadUnload(string_view_utf8 name)
    : DialogStateful<PhasesLoadUnload>(name, LoadUnloadFactory()) {}

// Phase callbacks to play a sound in specific moment at the start/end of
// specified phase
void DialogLoadUnload::phaseAlertSound() { Sound_Play(eSOUND_TYPE_SingleBeep); }
