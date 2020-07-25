#include "DialogLoadUnload.hpp"
#include "gui.hpp"    //resource_font
#include "resource.h" //IDR_FNT_BIG
#include "sound.hpp"
#include "../lang/i18n.h"
#include "dialog_response.hpp"

//all buttons share same Window, thus it must be static
static const RadioButton::Window radio_win = { resource_font(IDR_FNT_BIG), gui_defaults.color_back, IDialogStateful::get_radio_button_size() };

//shorter creation of single state
inline RadioButton btn(PhasesLoadUnload phase, const PhaseTexts &texts) {
    return RadioButton(radio_win, ClientResponses::GetResponses(phase), texts);
}

/*****************************************************************************/
// clang-format off
static const PhaseTexts ph_txt_stop          = { ResponseTexts::Get(Response::Stop),             ResponseTexts::Get(Response::_none), ResponseTexts::Get(Response::_none), ResponseTexts::Get(Response::_none) };
static const PhaseTexts ph_txt_cont          = { ResponseTexts::Get(Response::Continue),         ResponseTexts::Get(Response::_none), ResponseTexts::Get(Response::_none), ResponseTexts::Get(Response::_none) };
static const PhaseTexts ph_txt_reheat        = { ResponseTexts::Get(Response::Reheat),           ResponseTexts::Get(Response::_none), ResponseTexts::Get(Response::_none), ResponseTexts::Get(Response::_none) };
static const PhaseTexts ph_txt_disa          = { ResponseTexts::Get(Response::Filament_removed), ResponseTexts::Get(Response::_none), ResponseTexts::Get(Response::_none), ResponseTexts::Get(Response::_none) };
static const PhaseTexts ph_txt_none          = { ResponseTexts::Get(Response::_none),            ResponseTexts::Get(Response::_none), ResponseTexts::Get(Response::_none), ResponseTexts::Get(Response::_none) };
static const PhaseTexts ph_txt_yesno         = { ResponseTexts::Get(Response::Yes),              ResponseTexts::Get(Response::No),    ResponseTexts::Get(Response::_none), ResponseTexts::Get(Response::_none) };
static const PhaseTexts ph_txt_iscolor       = { ResponseTexts::Get(Response::Yes),              ResponseTexts::Get(Response::No),    ResponseTexts::Get(Response::Retry), ResponseTexts::Get(Response::_none) };
static const PhaseTexts ph_txt_iscolor_purge = { ResponseTexts::Get(Response::Yes),              ResponseTexts::Get(Response::No),    ResponseTexts::Get(Response::_none), ResponseTexts::Get(Response::_none) };

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
        DialogLoadUnload::State { txt_first,              btn(PhasesLoadUnload::_first,           ph_txt_none) },
        DialogLoadUnload::State { txt_parking,            btn(PhasesLoadUnload::Parking,          ph_txt_stop) },
        DialogLoadUnload::State { txt_wait_temp,          btn(PhasesLoadUnload::WaitingTemp,      ph_txt_stop) },
        DialogLoadUnload::State { txt_prep_ram,           btn(PhasesLoadUnload::PreparingToRam,   ph_txt_stop) },
        DialogLoadUnload::State { txt_ram,                btn(PhasesLoadUnload::Ramming,          ph_txt_stop) },
        DialogLoadUnload::State { txt_unload,             btn(PhasesLoadUnload::Unloading,        ph_txt_stop) },
        DialogLoadUnload::State { txt_unload,             btn(PhasesLoadUnload::RemoveFilament,   ph_txt_stop) },
        DialogLoadUnload::State { txt_push_fil,           btn(PhasesLoadUnload::UserPush,         ph_txt_cont), DialogLoadUnload::phaseAlertSound },
        DialogLoadUnload::State { txt_nozzle_cold,        btn(PhasesLoadUnload::NozzleTimeout,    ph_txt_reheat) },
        DialogLoadUnload::State { txt_make_sure_inserted, btn(PhasesLoadUnload::MakeSureInserted, ph_txt_cont), DialogLoadUnload::phaseAlertSound },
        DialogLoadUnload::State { txt_inserting,          btn(PhasesLoadUnload::Inserting,        ph_txt_stop) },
        DialogLoadUnload::State { txt_is_filament_in_gear,btn(PhasesLoadUnload::IsFilamentInGear, ph_txt_yesno) },
        DialogLoadUnload::State { txt_ejecting,           btn(PhasesLoadUnload::Ejecting,         ph_txt_none) },
        DialogLoadUnload::State { txt_loading,            btn(PhasesLoadUnload::Loading,          ph_txt_stop) },
        DialogLoadUnload::State { txt_purging,            btn(PhasesLoadUnload::Purging,          ph_txt_stop) },
        DialogLoadUnload::State { txt_is_color,           btn(PhasesLoadUnload::IsColor,          ph_txt_iscolor), DialogLoadUnload::phaseAlertSound },
        DialogLoadUnload::State { txt_is_color,           btn(PhasesLoadUnload::IsColorPurge,     ph_txt_iscolor_purge), DialogLoadUnload::phaseAlertSound },
        DialogLoadUnload::State { txt_unparking,          btn(PhasesLoadUnload::Unparking,        ph_txt_stop) },
    };
    return ret;
}
// clang-format on
/*****************************************************************************/

DialogLoadUnload::DialogLoadUnload(const char *name)
    : DialogStateful<PhasesLoadUnload>(name, LoadUnloadFactory()) {}

// Phase callbacks to play a sound in specific moment at the start/end of
// specified phase
void DialogLoadUnload::phaseAlertSound() { Sound_Play(eSOUND_TYPE_SingleBeep); }
