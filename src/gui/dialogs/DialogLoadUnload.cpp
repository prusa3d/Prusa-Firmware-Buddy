#include "DialogLoadUnload.hpp"
#include "gui.hpp" //resource_font
#include "sound.hpp"
#include "i18n.h"
#include "client_response_texts.hpp"

/*****************************************************************************/
// clang-format off
static const PhaseTexts ph_txt_reheat        = { BtnResponse::GetText(Response::Reheat),           BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none) };
static const PhaseTexts ph_txt_disa          = { BtnResponse::GetText(Response::Filament_removed), BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none) };
static const PhaseTexts ph_txt_iscolor       = { BtnResponse::GetText(Response::Yes),              BtnResponse::GetText(Response::No),    BtnResponse::GetText(Response::Retry), BtnResponse::GetText(Response::_none) };
static const PhaseTexts ph_txt_iscolor_purge = { BtnResponse::GetText(Response::Yes),              BtnResponse::GetText(Response::No),    BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none) };

static const char *txt_first              = N_("Finishing buffered gcodes.");
static const char *txt_parking            = N_("Parking");
static const char *txt_unparking          = N_("Unparking");
static const char *txt_wait_temp          = N_("Waiting for temperature");
static const char *txt_prep_ram           = N_("Preparing to ram");
static const char *txt_ram                = N_("Ramming");
static const char *txt_unload             = N_("Unloading");
static const char *txt_unload_confirm     = N_("Was filament unload successful?");
static const char *txt_filament_not_in_fs = N_("Please remove filament from filament sensor.");
static const char *txt_manual_unload      = N_("Please open idler and remove filament manually");
static const char *txt_push_fil           = N_("Press CONTINUE and push filament into the extruder.");
static const char *txt_make_sure_inserted = N_("Make sure the filament is inserted through the sensor.");
static const char *txt_inserting          = N_("Inserting");
static const char *txt_is_filament_in_gear= N_("Is filament in extruder gear?");
static const char *txt_ejecting           = N_("Ejecting");
static const char *txt_loading            = N_("Loading to nozzle");
static const char *txt_purging            = N_("Purging");
static const char *txt_is_color           = N_("Is color correct?");

/// indicator for M600 or filament runout phases
/// because this sound should be beeping only for those parts (M600 & runout)
bool DialogLoadUnload::is_M600_phase = false;

static DialogLoadUnload::States LoadUnloadFactory() {
    DialogLoadUnload::States ret = {
        DialogLoadUnload::State { txt_first,                ClientResponses::GetResponses(PhasesLoadUnload::_first),                        ph_txt_none },
        DialogLoadUnload::State { txt_parking,              ClientResponses::GetResponses(PhasesLoadUnload::Parking_stoppable),             ph_txt_stop },
        DialogLoadUnload::State { txt_parking,              ClientResponses::GetResponses(PhasesLoadUnload::Parking_unstoppable),           ph_txt_none },
        DialogLoadUnload::State { txt_wait_temp,            ClientResponses::GetResponses(PhasesLoadUnload::WaitingTemp_stoppable),         ph_txt_stop },
        DialogLoadUnload::State { txt_wait_temp,            ClientResponses::GetResponses(PhasesLoadUnload::WaitingTemp_unstoppable),       ph_txt_none },
        DialogLoadUnload::State { txt_prep_ram,             ClientResponses::GetResponses(PhasesLoadUnload::PreparingToRam_stoppable),      ph_txt_stop },
        DialogLoadUnload::State { txt_prep_ram,             ClientResponses::GetResponses(PhasesLoadUnload::PreparingToRam_unstoppable),    ph_txt_none },
        DialogLoadUnload::State { txt_ram,                  ClientResponses::GetResponses(PhasesLoadUnload::Ramming_stoppable),             ph_txt_stop },
        DialogLoadUnload::State { txt_ram,                  ClientResponses::GetResponses(PhasesLoadUnload::Ramming_unstoppable),           ph_txt_none },
        DialogLoadUnload::State { txt_unload,               ClientResponses::GetResponses(PhasesLoadUnload::Unloading_stoppable),           ph_txt_stop },
        DialogLoadUnload::State { txt_unload,               ClientResponses::GetResponses(PhasesLoadUnload::Unloading_unstoppable),         ph_txt_none },
        DialogLoadUnload::State { txt_unload,               ClientResponses::GetResponses(PhasesLoadUnload::RemoveFilament),                ph_txt_stop },
        DialogLoadUnload::State { txt_unload_confirm,       ClientResponses::GetResponses(PhasesLoadUnload::IsFilamentUnloaded),            ph_txt_yesno, DialogLoadUnload::phaseWaitSound },
        DialogLoadUnload::State { txt_filament_not_in_fs,   ClientResponses::GetResponses(PhasesLoadUnload::FilamentNotInFS),               ph_txt_none, DialogLoadUnload::phaseAlertSound},
        DialogLoadUnload::State { txt_manual_unload,        ClientResponses::GetResponses(PhasesLoadUnload::ManualUnload),                  ph_txt_continue, DialogLoadUnload::phaseStopSound },
        DialogLoadUnload::State { txt_push_fil,             ClientResponses::GetResponses(PhasesLoadUnload::UserPush_stoppable),            ph_txt_continue_stop, DialogLoadUnload::phaseAlertSound },
        DialogLoadUnload::State { txt_push_fil,             ClientResponses::GetResponses(PhasesLoadUnload::UserPush_unstoppable),          ph_txt_continue, DialogLoadUnload::phaseAlertSound },
        DialogLoadUnload::State { txt_make_sure_inserted,   ClientResponses::GetResponses(PhasesLoadUnload::MakeSureInserted_stoppable),    ph_txt_stop, DialogLoadUnload::phaseAlertSound },
        DialogLoadUnload::State { txt_make_sure_inserted,   ClientResponses::GetResponses(PhasesLoadUnload::MakeSureInserted_unstoppable),  ph_txt_none, DialogLoadUnload::phaseAlertSound },
        DialogLoadUnload::State { txt_inserting,            ClientResponses::GetResponses(PhasesLoadUnload::Inserting_stoppable),           ph_txt_stop },
        DialogLoadUnload::State { txt_inserting,            ClientResponses::GetResponses(PhasesLoadUnload::Inserting_unstoppable),         ph_txt_none },
        DialogLoadUnload::State { txt_is_filament_in_gear,  ClientResponses::GetResponses(PhasesLoadUnload::IsFilamentInGear),              ph_txt_yesno },
        DialogLoadUnload::State { txt_ejecting,             ClientResponses::GetResponses(PhasesLoadUnload::Ejecting_stoppable),            ph_txt_stop },
        DialogLoadUnload::State { txt_ejecting,             ClientResponses::GetResponses(PhasesLoadUnload::Ejecting_unstoppable),          ph_txt_none },
        DialogLoadUnload::State { txt_loading,              ClientResponses::GetResponses(PhasesLoadUnload::Loading_stoppable),             ph_txt_stop },
        DialogLoadUnload::State { txt_loading,              ClientResponses::GetResponses(PhasesLoadUnload::Loading_unstoppable),           ph_txt_none },
        DialogLoadUnload::State { txt_purging,              ClientResponses::GetResponses(PhasesLoadUnload::Purging_stoppable),             ph_txt_stop },
        DialogLoadUnload::State { txt_purging,              ClientResponses::GetResponses(PhasesLoadUnload::Purging_unstoppable),           ph_txt_none },
        DialogLoadUnload::State { txt_is_color,             ClientResponses::GetResponses(PhasesLoadUnload::IsColor),                       ph_txt_iscolor, DialogLoadUnload::phaseAlertSound },
        DialogLoadUnload::State { txt_is_color,             ClientResponses::GetResponses(PhasesLoadUnload::IsColorPurge),                  ph_txt_iscolor_purge, DialogLoadUnload::phaseAlertSound },
        DialogLoadUnload::State { txt_unparking,            ClientResponses::GetResponses(PhasesLoadUnload::Unparking),                     ph_txt_stop },
    };

    return ret;
}
// clang-format on
/*****************************************************************************/

DialogLoadUnload::DialogLoadUnload(string_view_utf8 name)
    : DialogStateful<PhasesLoadUnload>(name, LoadUnloadFactory(), has_footer::yes)
    , footer(this) {}

// Phase callbacks to play a sound in specific moment at the start/end of
// specified phase
void DialogLoadUnload::phaseAlertSound() {
    Sound_Stop();
    Sound_Play(eSOUND_TYPE::SingleBeep);
}
void DialogLoadUnload::phaseWaitSound() {
    if (DialogLoadUnload::is_M600_phase) { /// this sound should be beeping only for M600 || runout
        Sound_Play(eSOUND_TYPE::WaitingBeep);
    }
}
void DialogLoadUnload::phaseStopSound() { Sound_Stop(); }
