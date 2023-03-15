#include "DialogLoadUnload.hpp"
#include "gui.hpp" //resource_font
#include "sound.hpp"
#include "i18n.h"
#include "client_response_texts.hpp"
#include "ScreenHandler.hpp"
#include "resource.h"
#include "mmu2_error_converter.h"
#include "filament_sensors_handler.hpp"
#include "png_resources.hpp"

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

#if HAS_MMU2
// MMU-related
static const char *txt_mmu_engag_idler    = N_("Engaging idler");
static const char *txt_mmu_diseng_idler   = N_("Disengaging idler");
static const char *txt_mmu_unload_finda   = N_("Unloading to FINDA");
static const char *txt_mmu_unload_pulley  = N_("Unloading to pulley");
static const char *txt_mmu_feed_finda     = N_("Feeding to FINDA");
static const char *txt_mmu_feed_bondtech  = N_("Feeding to drive gear");
static const char *txt_mmu_feed_nozzle    = N_("Feeding to nozzle");
static const char *txt_mmu_avoid_grind    = N_("Avoiding grind");
static const char *txt_mmu_finish_moves   = N_("Finishing moves");
static const char *txt_mmu_err_internal   = N_("ERR Internal");
static const char *txt_mmu_err_help_fil   = N_("ERR Helping filament");
static const char *txt_mmu_err_tmc        = N_("ERR TMC failed");
static const char *txt_mmu_unload_filament= N_("Unloading filament");
static const char *txt_mmu_load_filament  = N_("Loading filament");
static const char *txt_mmu_select_slot    = N_("Selecting filament slot");
static const char *txt_mmu_prepare_blade  = N_("Preparing blade");
static const char *txt_mmu_push_filament  = N_("Pushing filament");
static const char *txt_mmu_perform_cut    = N_("Performing cut");
static const char *txt_mmu_return_selector= N_("Returning selector");
static const char *txt_mmu_park_selector  = N_("Parking selector");
static const char *txt_mmu_eject_filament = N_("Ejecting filament");
static const char *txt_mmu_retract_finda  = N_("Retracting from FINDA");

//MMU_ErrWaitForUser, // need to distinguish error states based on prusa-error-codes @@TODO
static const char *txt_mmu_err_wait_user  = N_("Waiting for user input");
#endif

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
#if HAS_MMU2
        DialogLoadUnload::State { txt_mmu_engag_idler,      ClientResponses::GetResponses(PhasesLoadUnload::MMU_EngagingIdler),     ph_txt_none },
        DialogLoadUnload::State { txt_mmu_diseng_idler,     ClientResponses::GetResponses(PhasesLoadUnload::MMU_DisengagingIdler),  ph_txt_none },
        DialogLoadUnload::State { txt_mmu_unload_finda,     ClientResponses::GetResponses(PhasesLoadUnload::MMU_UnloadingToFinda),  ph_txt_none },
        DialogLoadUnload::State { txt_mmu_unload_pulley,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_UnloadingToPulley), ph_txt_none },
        DialogLoadUnload::State { txt_mmu_feed_finda,       ClientResponses::GetResponses(PhasesLoadUnload::MMU_FeedingToFinda),    ph_txt_none },
        DialogLoadUnload::State { txt_mmu_feed_bondtech,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_FeedingToBondtech), ph_txt_none },
        DialogLoadUnload::State { txt_mmu_feed_nozzle,      ClientResponses::GetResponses(PhasesLoadUnload::MMU_FeedingToNozzle),   ph_txt_none },
        DialogLoadUnload::State { txt_mmu_avoid_grind,      ClientResponses::GetResponses(PhasesLoadUnload::MMU_AvoidingGrind),     ph_txt_none },
        DialogLoadUnload::State { txt_mmu_finish_moves,     ClientResponses::GetResponses(PhasesLoadUnload::MMU_FinishingMoves),    ph_txt_none },
        DialogLoadUnload::State { txt_mmu_diseng_idler,     ClientResponses::GetResponses(PhasesLoadUnload::MMU_ERRDisengagingIdler),ph_txt_none },
        DialogLoadUnload::State { txt_mmu_engag_idler,      ClientResponses::GetResponses(PhasesLoadUnload::MMU_ERREngagingIdler),  ph_txt_none },

        // the one and only MMU Error screen
        DialogLoadUnload::State { txt_mmu_err_wait_user,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_ERRWaitingForUser), ph_txt_none },

        DialogLoadUnload::State { txt_mmu_err_internal,     ClientResponses::GetResponses(PhasesLoadUnload::MMU_ERRInternal),       ph_txt_none },
        DialogLoadUnload::State { txt_mmu_err_help_fil,     ClientResponses::GetResponses(PhasesLoadUnload::MMU_ERRHelpingFilament),ph_txt_none },
        DialogLoadUnload::State { txt_mmu_err_tmc,          ClientResponses::GetResponses(PhasesLoadUnload::MMU_ERRTMCFailed),      ph_txt_none },
        DialogLoadUnload::State { txt_mmu_unload_filament,  ClientResponses::GetResponses(PhasesLoadUnload::MMU_UnloadingFilament), ph_txt_none },
        DialogLoadUnload::State { txt_mmu_load_filament,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_LoadingFilament),   ph_txt_none },
        DialogLoadUnload::State { txt_mmu_select_slot,      ClientResponses::GetResponses(PhasesLoadUnload::MMU_SelectingFilamentSlot),ph_txt_none },
        DialogLoadUnload::State { txt_mmu_prepare_blade,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_PreparingBlade),    ph_txt_none },
        DialogLoadUnload::State { txt_mmu_push_filament,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_PushingFilament),   ph_txt_none },
        DialogLoadUnload::State { txt_mmu_perform_cut,      ClientResponses::GetResponses(PhasesLoadUnload::MMU_PerformingCut),     ph_txt_none },
        DialogLoadUnload::State { txt_mmu_return_selector,  ClientResponses::GetResponses(PhasesLoadUnload::MMU_ReturningSelector), ph_txt_none },
        DialogLoadUnload::State { txt_mmu_park_selector,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_ParkingSelector),   ph_txt_none },
        DialogLoadUnload::State { txt_mmu_eject_filament,   ClientResponses::GetResponses(PhasesLoadUnload::MMU_EjectingFilament),  ph_txt_none },
        DialogLoadUnload::State { txt_mmu_retract_finda,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_RetractingFromFinda),ph_txt_none },
#endif
    };

    return ret;
}
// clang-format on
/*****************************************************************************/

static constexpr Rect16 mmu_title_rect = { 14, 44, 317, 22 };
static constexpr Rect16 mmu_desc_rect = { 14, 66, 224, 105 };
static constexpr Rect16 mmu_icon_rect = { 263, 73, 59, 72 };
static constexpr Rect16 mmu_link_rect = { 14, 165, 317, 48 };
static constexpr Rect16 mmu_qr_rect = { 341, 44, 125, 125 };
static constexpr char error_code_link_format[] = N_("More detail at\nhelp.prusa3d.com/%u");

DialogLoadUnload::DialogLoadUnload(string_view_utf8 name)
    : AddSuperWindow<DialogStateful<PhasesLoadUnload>>(name, LoadUnloadFactory(), has_footer::yes)
#if FOOTER_ITEMS_PER_LINE__ >= 5
    , footer(this, footer::items::ItemNozzle, footer::items::ItemBed, footer::items::ItemFSensor,
    #if HAS_MMU2
          FSensors_instance().HasMMU() ? footer::items::ItemFinda : footer::items::count_ // finda in MMU mode, nothing othervise
    #else
          footer::items::count_
    #endif
#else
    , footer(this
#endif
          )
    , radio_for_red_screen(this, GuiDefaults::GetIconnedButtonRect(GetRect()))
    , text_link(this, mmu_link_rect, is_multiline::yes, is_closed_on_click_t::no)
    , icon_hand(this, mmu_icon_rect, &png::hand_qr_59x72)
    , qr(this, mmu_qr_rect) {

    text_link.font = resource_font(IDR_FNT_SMALL);

    radio_for_red_screen.SetHasIcon();
    radio_for_red_screen.Hide();
    text_link.Hide();
    icon_hand.Hide();
    qr.Hide();
}

DialogLoadUnload::~DialogLoadUnload() {
    // Dtor only resets the header to black.
    // The header could not be red before the screen opened.
    // And even if it were, this behavior would only cause the header to appear in the wrong color.
    // If this starts to cause any problems, it is possible to send an event from ctor to ask for the status of the header
    // and the header could respond through another event.
    // Or we could just implement a color stack in the header.
    event_conversion_union uni;
    uni.header.layout = layout_color::black;
    Screens::Access()->ScreenEvent(this, GUI_event_t::HEADER_COMMAND, uni.pvoid);
}

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

#if HAS_MMU2
static constexpr bool isRed(uint8_t phs) {
    int16_t err_pos = int16_t(PhasesLoadUnload::MMU_ERRWaitingForUser) - int16_t(PhasesLoadUnload::_first);
    return phs == err_pos;
}
#endif

bool DialogLoadUnload::change(uint8_t phs, fsm::PhaseData data) {
#if HAS_MMU2
    //was black, is red
    if (!isRed(phase) && isRed(phs)) {
        SetRedLayout();
        //this dialog does not contain header, so it broadcasts event to all windows
        event_conversion_union uni;
        uni.header.layout = layout_color::red;
        Screens::Access()->ScreenEvent(this, GUI_event_t::HEADER_COMMAND, uni.pvoid);

        title.SetRect(mmu_title_rect);

        progress.Hide();

        label.SetRect(mmu_desc_rect);
        label.SetFont(resource_font(IDR_FNT_SMALL));

        radio_for_red_screen.Show();               // show red screen radio button
        CaptureNormalWindow(radio_for_red_screen); // capture red screen radio button
        radio.Hide();                              // hide normal radio button

        text_link.Show();
        icon_hand.Show();
        qr.Show();
    }

    //is red
    if (isRed(phs)) {
        if (!can_change(phs))
            return false;

        const MMU2::MMUErrorDesc *ptr_desc = fsm::PointerSerializer<MMU2::MMUErrorDesc>(data).Get();

        red_screen_update(*ptr_desc);

        phase = phs; // set it directly, do not use super::change(phs, data);

        return true;
    }

    //was red, is black
    if (isRed(phase) && !isRed(phs)) {
        title.SetRect(get_title_rect(GetRect()));
        SetBlackLayout();
        //this dialog does not contain header, so it broadcasts event to all windows
        event_conversion_union uni;
        uni.header.layout = layout_color::black;
        Screens::Access()->ScreenEvent(this, GUI_event_t::HEADER_COMMAND, uni.pvoid);

        progress.Show();

        label.SetRect(get_label_rect(GetRect(), has_footer::yes));
        label.SetFont(GuiDefaults::Font);

        radio.Show();                // show normal radio button
        CaptureNormalWindow(radio);  // capture normal radio button
        radio_for_red_screen.Hide(); // hide red screen radio button

        text_link.Hide();
        icon_hand.Hide();
        qr.Hide();
    }
#endif

    //is black
    return super::change(phs, data);
}

void DialogLoadUnload::red_screen_update(const MMU2::MMUErrorDesc &err) {
    responses[0] = ConvertMMUButtonOperation(err.buttons[0]);
    responses[1] = ConvertMMUButtonOperation(err.buttons[1]);
    responses[2] = ConvertMMUButtonOperation(err.buttons[2]);

    radio_for_red_screen.Change(responses);

    title.SetText(string_view_utf8::MakeRAM((const uint8_t *)err.err_title));
    label.SetText(string_view_utf8::MakeRAM((const uint8_t *)err.err_text));

    // 32 is the length of the string without link number at the end
    snprintf(error_code_str, 32 + MaxErrorCodeDigits + 1, error_code_link_format, (uint16_t)err.err_num);
    text_link.SetText(string_view_utf8::MakeRAM((const uint8_t *)error_code_str));

    qr.SetQRHeader(err.err_num);
}
