#include "DialogLoadUnload.hpp"
#include "gui.hpp" //resource_font
#include "sound.hpp"
#include "i18n.h"
#include "client_response_texts.hpp"
#include "ScreenHandler.hpp"
#include "fonts.hpp"
#include "mmu2_error_converter.h"
#include "filament_sensors_handler.hpp"
#include "img_resources.hpp"
#include "fsm_loadunload_type.hpp"
#include <option/has_side_fsensor.h>
#include <option/has_mmu2.h>

RadioButtonNotice::RadioButtonNotice(window_t *parent, Rect16 rect)
    : AddSuperWindow<RadioButton>(parent, rect) {}

void RadioButtonNotice::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK: {
        Response response = Click();
        marlin_client::FSM_response(current_phase, response);
        break;
    }
    default:
        SuperWindowEvent(sender, event, param);
    }
}

void RadioButtonNotice::ChangePhase(PhasesLoadUnload phase, PhaseResponses responses) {
    current_phase = phase;
    Change(responses);
}

/*****************************************************************************/
// clang-format off
static const PhaseTexts ph_txt_reheat        = { BtnResponse::GetText(Response::Reheat),           BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none) };
static const PhaseTexts ph_txt_disa          = { BtnResponse::GetText(Response::Filament_removed), BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none) };
static const PhaseTexts ph_txt_iscolor       = { BtnResponse::GetText(Response::Yes),              BtnResponse::GetText(Response::No),    BtnResponse::GetText(Response::Retry), BtnResponse::GetText(Response::_none) };
static const PhaseTexts ph_txt_iscolor_purge = { BtnResponse::GetText(Response::Yes),              BtnResponse::GetText(Response::No),    BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none) };
static const PhaseTexts ph_txt_unload        = { BtnResponse::GetText(Response::Unload),           BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none), BtnResponse::GetText(Response::_none) };

static const char *txt_first              = N_("Finishing buffered gcodes.");
static const char *txt_tool               = N_("Changing tool");
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
#if HAS_LOADCELL()
static const char *txt_filament_stuck     = ""; // Empty here, set from the error description
#endif
#if HAS_MMU2()
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
static const char *txt_mmu_homing         = N_("Homing");
static const char *txt_mmu_moving_selector= N_("Moving selector");
static const char *txt_mmu_feeding_fsensor= N_("Feeding to fsensor");
static const char *txt_mmu_hw_test_begin  = N_("HW test begin");
static const char *txt_mmu_hw_test_idler  = N_("HW test idler");
static const char *txt_mmu_hw_test_sel    = N_("HW test selector");
static const char *txt_mmu_hw_test_pulley = N_("HW test pulley");
static const char *txt_mmu_hw_test_cleanup= N_("HW test cleanup");
static const char *txt_mmu_hw_test_exec   = N_("HW test exec");
static const char *txt_mmu_hw_test_display= N_("HW test display");
static const char *txt_mmu_errhw_test_fail= N_("ERR HW test failed");
static const char *txt_mmu_insert_filament= N_("Press CONTINUE and push filament into MMU.");

//MMU_ErrWaitForUser, // need to distinguish error states based on prusa-error-codes @@TODO
static const char *txt_mmu_err_wait_user  = N_("Waiting for user input");
#endif

static DialogLoadUnload::States LoadUnloadFactory() {
    DialogLoadUnload::States ret = {
        DialogLoadUnload::State { txt_first,                ClientResponses::GetResponses(PhasesLoadUnload::_first),                        ph_txt_none },
        DialogLoadUnload::State { txt_tool,                 ClientResponses::GetResponses(PhasesLoadUnload::ChangingTool),                  ph_txt_none },
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
#if HAS_LOADCELL()
        DialogLoadUnload::State { txt_filament_stuck,       ClientResponses::GetResponses(PhasesLoadUnload::FilamentStuck),                 ph_txt_unload, DialogLoadUnload::phaseAlertSound },
#endif
#if HAS_MMU2()
        DialogLoadUnload::State { txt_mmu_insert_filament,  ClientResponses::GetResponses(PhasesLoadUnload::LoadFilamentIntoMMU),   ph_txt_none }, // TODO how the button is Continue
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
        DialogLoadUnload::State { txt_mmu_homing,           ClientResponses::GetResponses(PhasesLoadUnload::MMU_Homing),            ph_txt_none },
        DialogLoadUnload::State { txt_mmu_moving_selector,  ClientResponses::GetResponses(PhasesLoadUnload::MMU_MovingSelector),    ph_txt_none },
        DialogLoadUnload::State { txt_mmu_feeding_fsensor,  ClientResponses::GetResponses(PhasesLoadUnload::MMU_FeedingToFSensor),  ph_txt_none },
        DialogLoadUnload::State { txt_mmu_hw_test_begin,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_HWTestBegin),       ph_txt_none },
        DialogLoadUnload::State { txt_mmu_hw_test_idler,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_HWTestIdler),       ph_txt_none },
        DialogLoadUnload::State { txt_mmu_hw_test_sel,      ClientResponses::GetResponses(PhasesLoadUnload::MMU_HWTestSelector),    ph_txt_none },
        DialogLoadUnload::State { txt_mmu_hw_test_pulley,   ClientResponses::GetResponses(PhasesLoadUnload::MMU_HWTestPulley),      ph_txt_none },
        DialogLoadUnload::State { txt_mmu_hw_test_cleanup,  ClientResponses::GetResponses(PhasesLoadUnload::MMU_HWTestCleanup),     ph_txt_none },
        DialogLoadUnload::State { txt_mmu_hw_test_exec,     ClientResponses::GetResponses(PhasesLoadUnload::MMU_HWTestExec),        ph_txt_none },
        DialogLoadUnload::State { txt_mmu_hw_test_display,  ClientResponses::GetResponses(PhasesLoadUnload::MMU_HWTestDisplay),     ph_txt_none },
        DialogLoadUnload::State { txt_mmu_errhw_test_fail,  ClientResponses::GetResponses(PhasesLoadUnload::MMU_ErrHwTestFailed),   ph_txt_none },
#endif
    };

    return ret;
}
// clang-format on
/*****************************************************************************/

static constexpr Rect16 notice_title_rect = { 86, 44, 374, 22 };
static constexpr Rect16 notice_text_rect = { 86, 72, 244, 140 };
static constexpr Rect16 notice_link_rect = { 86, 218, 244, 32 };
static constexpr Rect16 notice_icon_rect = { 370, 180, 59, 72 };
static constexpr Rect16 notice_icon_type_rect = { 24, 44, 48, 48 };
static constexpr Rect16 notice_qr_rect = { 350, 72, 100, 100 };
static constexpr char error_code_link_format[] = N_("More detail at\nprusa.io/%05u");
namespace {
constexpr size_t color_size { 16 };
constexpr size_t text_height { 21 };
constexpr size_t text_margin { 18 };
constexpr size_t top_of_bottom_part { GuiDefaults::ScreenHeight - GuiDefaults::FooterHeight - GuiDefaults::FramePadding - GuiDefaults::ButtonHeight - 5 };
constexpr Rect16 filament_color_icon_rect { 0, top_of_bottom_part - text_height + (text_height - color_size) / 2, color_size, color_size }; // x needs to be 0, to be set later
constexpr Rect16 filament_type_text_rect { text_margin, top_of_bottom_part - text_height, GuiDefaults::ScreenWidth - 2 * text_margin, 21 };
} // namespace

DialogLoadUnload::DialogLoadUnload(fsm::BaseData data)
    : AddSuperWindow<DialogStateful<PhasesLoadUnload>>(get_name(ProgressSerializerLoadUnload(data.GetData()).mode), LoadUnloadFactory(), has_footer::yes)
    , footer(this
#if FOOTER_ITEMS_PER_LINE__ >= 5
          ,
          footer::Item::nozzle, footer::Item::bed, footer::Item::f_sensor
    #if HAS_MMU2()
          ,
          FSensors_instance().HasMMU() ? footer::Item::finda : footer::Item::none,
          FSensors_instance().HasMMU() ? footer::Item::f_s_value : footer::Item::none
    #elif HAS_SIDE_FSENSOR()
          ,
          footer::Item::f_sensor_side
    #else
          ,
          footer::Item::none
    #endif
#endif
          )
    , notice_frame(this, get_frame_rect(GetRect(), has_footer::yes))
    , notice_title(&notice_frame, notice_title_rect, is_multiline::no)
    , notice_text(&notice_frame, notice_text_rect, is_multiline::yes)
    , notice_link(&notice_frame, notice_link_rect, is_multiline::yes)
    , notice_icon_hand(&notice_frame, notice_icon_rect, &img::hand_qr_59x72)
    , notice_icon_type(&notice_frame, notice_icon_type_rect, &img::warning_48x48)
    , notice_qr(&notice_frame, notice_qr_rect)
    , notice_radio_button(&notice_frame, GuiDefaults::GetButtonRect_AvoidFooter(GetRect()))
    , filament_type_text(&progress_frame, filament_type_text_rect, is_multiline::no)
    , filament_color_icon(&progress_frame, filament_color_icon_rect)
    , mode(ProgressSerializerLoadUnload(data.GetData()).mode) {
    notice_frame.CaptureNormalWindow(notice_radio_button);

    notice_title.set_font(GuiDefaults::FontBig);

    notice_text.set_font(resource_font(IDR_FNT_SPECIAL));

    filament_type_text.SetAlignment(Align_t::Center());
    filament_color_icon.SetRoundCorners();
    instance = this;

    notice_link.set_font(resource_font(IDR_FNT_SMALL));

    notice_frame.Hide(); // default state is 'normal' - leave progress frame the only one shown

    Change(data);
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
    instance = nullptr;
}

DialogLoadUnload *DialogLoadUnload::instance = nullptr;

// Phase callbacks to play a sound in specific moment at the start/end of
// specified phase
void DialogLoadUnload::phaseAlertSound() {
    Sound_Stop();
    Sound_Play(eSOUND_TYPE::SingleBeep);
}
void DialogLoadUnload::phaseWaitSound() {
    if (instance && (instance->get_mode() == LoadUnloadMode::Change || instance->get_mode() == LoadUnloadMode::FilamentStuck)) { /// this sound should be beeping only for M600 || runout
        Sound_Play(eSOUND_TYPE::WaitingBeep);
    }
}
void DialogLoadUnload::phaseStopSound() { Sound_Stop(); }

static constexpr bool is_notice_mmu([[maybe_unused]] PhasesLoadUnload phase) {
#if HAS_MMU2()
    return phase == PhasesLoadUnload::MMU_ERRWaitingForUser;
#else
    return false;
#endif
}

static constexpr bool is_notice_fstuck([[maybe_unused]] PhasesLoadUnload phase) {
#if HAS_LOADCELL()
    return phase == PhasesLoadUnload::FilamentStuck;
#else
    return false;
#endif
}

static constexpr bool is_notice(PhasesLoadUnload phase) {
    return is_notice_mmu(phase) || is_notice_fstuck(phase);
}

bool DialogLoadUnload::change(PhasesLoadUnload phase, fsm::PhaseData data) {
    LoadUnloadMode new_mode = ProgressSerializerLoadUnload(data).mode;
    if (new_mode != mode) {
        mode = new_mode;
        title.SetText(get_name(mode));
    }

#if HAS_MMU2() || HAS_LOADCELL()
    // was normal (or uninitialized), is notice
    if ((!current_phase || !is_notice(*current_phase)) && is_notice(phase)) {
        progress_frame.Hide();
        notice_frame.Show();
        CaptureNormalWindow(notice_frame);
    }

    // is notice
    if (is_notice(phase)) {
        if (!can_change(phase)) {
            return false;
        }

    #if HAS_MMU2()
        if (is_notice_mmu(phase)) {
            const MMU2::MMUErrDesc *ptr_desc = fsm::PointerSerializer<MMU2::MMUErrDesc>(data).Get();
            PhaseResponses responses {
                ButtonOperationToResponse(ptr_desc->buttons[0]),
                ButtonOperationToResponse(ptr_desc->buttons[1]),
                ButtonOperationToResponse(ptr_desc->buttons[2])
            };

            notice_radio_button.set_fixed_width_buttons_count(3);
            notice_radio_button.ChangePhase(phase, responses);
            notice_update(ftrstd::to_underlying(ptr_desc->err_code), ptr_desc->err_title, ptr_desc->err_text, ptr_desc->type);
        }
    #endif
    #if HAS_LOADCELL()
        // here, an "else" would be nice, but there might be printers with MMU and without loadcell in the future...
        if (is_notice_fstuck(phase)) {
            // An ugly workaround to abuse existing infrastructure - this is not an MMU-related error
            // yet we need to throw a dialog with a QR code and a button.
            auto err_desc = find_error(ErrCode::ERR_MECHANICAL_STUCK_FILAMENT_DETECTED);

            notice_radio_button.set_fixed_width_buttons_count(0);
            notice_radio_button.ChangePhase(phase, { Response::Unload });
            notice_update(ftrstd::to_underlying(err_desc.err_code), err_desc.err_title, err_desc.err_text, MMU2::ErrType::WARNING);
        }
    #endif
        current_phase = phase; // set it directly, do not use super::change(phase, data);

        return true;
    }

    // was notice (or uninitialized), is normal
    if ((!current_phase || is_notice(*current_phase)) && !is_notice(phase)) {
        progress_frame.Show();
        notice_frame.Hide();
        CaptureNormalWindow(progress_frame);
    }
#endif

    // is normal
    return super::change(phase, data);
}

void DialogLoadUnload::notice_update(uint16_t errCode, const char *errTitle, const char *errDesc, MMU2::ErrType type) {
    switch (type) {
    case MMU2::ErrType::ERROR:
        notice_icon_type.SetRes(&img::error_48x48);
        break;
    case MMU2::ErrType::WARNING:
        notice_icon_type.SetRes(&img::warning_48x48);
        break;
    case MMU2::ErrType::USER_ACTION:
        notice_icon_type.SetRes(&img::info_48x48);
        break;
    }

    notice_title.SetText(string_view_utf8::MakeRAM((const uint8_t *)errTitle));
    notice_text.SetText(string_view_utf8::MakeRAM((const uint8_t *)errDesc));

    snprintf(error_code_str, sizeof(error_code_str), error_code_link_format, errCode);
    notice_link.SetText(string_view_utf8::MakeRAM((const uint8_t *)error_code_str));

    notice_qr.SetQRHeader(errCode);
}

constexpr static const char title_change[] = N_("Changing filament");
constexpr static const char title_load[] = N_("Loading filament");
constexpr static const char title_unload[] = N_("Unloading filament");
constexpr static const char title_purge[] = N_("Purging filament");
constexpr static const char title_test[] = N_("Testing filament");
constexpr static const char title_index_error[] = "Index error"; // intentionally not to be translated

string_view_utf8 DialogLoadUnload::get_name(LoadUnloadMode mode) {
    switch (mode) {
    case LoadUnloadMode::Change:
        return _(title_change);
    case LoadUnloadMode::Load:
        return _(title_load);
    case LoadUnloadMode::Unload:
        return _(title_unload);
    case LoadUnloadMode::Purge:
        return _(title_purge);
    case LoadUnloadMode::Test:
        return _(title_test);
    default:
        break;
    }
    return string_view_utf8::MakeCPUFLASH((const uint8_t *)title_index_error);
}

float DialogLoadUnload::deserialize_progress(fsm::PhaseData data) const {
    return ProgressSerializerLoadUnload(data).progress;
}

void DialogLoadUnload::phaseEnter() {
    if (!current_phase) {
        return;
    }
    AddSuperWindow<DialogStateful<PhasesLoadUnload>>::phaseEnter();

    if (mode == LoadUnloadMode::Load) { // Change is currently split into Load/Unload, therefore no need to if (mode == change)
        if (filament::get_type_to_load() != filament::Type::NONE) {
            filament_type_text.Show();
            auto fil_name = filament::get_description(filament::get_type_to_load()).name;
            filament_type_text.SetText(_(fil_name));
            if (filament::get_color_to_load().has_value()) {

                int16_t left_pos = (GuiDefaults::ScreenWidth - (resource_font_size(IDR_FNT_NORMAL).w + 1) * (strlen(fil_name) + 1 + 1) - color_size) / 2; // make the pos to be on the left of the text (+ one added space to the left of the text, + additional one for some reason makes it work )
                auto rect = filament_color_icon_rect + Rect16::X_t { static_cast<int16_t>(left_pos) };

                auto col = filament::get_color_to_load().value();
                filament_color_icon.SetBackColor(to_color_t(col.red, col.green, col.blue));
                filament_color_icon.SetRect(rect);
                filament_color_icon.Show();
            } else {
                filament_color_icon.Hide();
            }
        } else {
            filament_type_text.Hide();
            filament_color_icon.Hide();
        }
    } else {
        filament_color_icon.Hide();
        filament_type_text.Hide();
    }
}
