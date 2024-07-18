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
#include <find_error.hpp>
#include <filament_to_load.hpp>

RadioButtonNotice::RadioButtonNotice(window_t *parent, Rect16 rect)
    : RadioButton(parent, rect) {}

void RadioButtonNotice::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK: {
        Response response = Click();
        marlin_client::FSM_response(current_phase, response);
        break;
    }
    default:
        RadioButton::windowEvent(sender, event, param);
    }
}

void RadioButtonNotice::ChangePhase(PhasesLoadUnload phase, PhaseResponses responses) {
    current_phase = phase;
    Change(responses);
}

/*****************************************************************************/
// clang-format off
static const PhaseTexts ph_txt_stop          = { get_response_text(Response::Stop),             get_response_text(Response::_none), get_response_text(Response::_none), get_response_text(Response::_none) };
static const PhaseTexts ph_txt_continue      = { get_response_text(Response::Continue),         get_response_text(Response::_none), get_response_text(Response::_none), get_response_text(Response::_none) };
static const PhaseTexts ph_txt_continue_stop = { get_response_text(Response::Continue),         get_response_text(Response::Stop),  get_response_text(Response::_none), get_response_text(Response::_none) };
static const PhaseTexts ph_txt_none          = { get_response_text(Response::_none),            get_response_text(Response::_none), get_response_text(Response::_none), get_response_text(Response::_none) };
static const PhaseTexts ph_txt_yesno         = { get_response_text(Response::Yes),              get_response_text(Response::No),    get_response_text(Response::_none), get_response_text(Response::_none) };
static const PhaseTexts ph_txt_iscolor       = { get_response_text(Response::Yes),              get_response_text(Response::No),    get_response_text(Response::Retry), get_response_text(Response::_none) };
static const PhaseTexts ph_txt_iscolor_purge = { get_response_text(Response::Yes),              get_response_text(Response::No),    get_response_text(Response::_none), get_response_text(Response::_none) };
static const PhaseTexts ph_txt_unload        = { get_response_text(Response::Unload),           get_response_text(Response::_none), get_response_text(Response::_none), get_response_text(Response::_none) };

static constexpr const char *txt_first              = N_("Finishing buffered gcodes");
static constexpr const char *txt_tool               = N_("Changing tool");
static constexpr const char *txt_parking            = N_("Parking");
static constexpr const char *txt_unparking          = N_("Unparking");
static constexpr const char *txt_wait_temp          = N_("Waiting for temperature");
static constexpr const char *txt_prep_ram           = N_("Preparing to ram");
static constexpr const char *txt_ram                = N_("Ramming");
static constexpr const char *txt_unload             = N_("Unloading");
static constexpr const char *txt_unload_confirm     = N_("Was filament unload successful?");
static constexpr const char *txt_filament_not_in_fs = N_("Please remove filament from filament sensor.");
static constexpr const char *txt_manual_unload      = N_("Please open idler and remove filament manually");
static constexpr const char *txt_push_fil           = N_("Press CONTINUE and push filament into the extruder.");
static constexpr const char *txt_make_sure_inserted = N_("Make sure the filament is inserted through the sensor.");
static constexpr const char *txt_inserting          = N_("Inserting");
static constexpr const char *txt_is_filament_in_gear= N_("Is filament in extruder gear?");
static constexpr const char *txt_ejecting           = N_("Ejecting");
static constexpr const char *txt_loading            = N_("Loading to nozzle");
static constexpr const char *txt_purging            = N_("Purging");
static constexpr const char *txt_is_color           = N_("Is color correct?");
#if HAS_LOADCELL()
static constexpr const char *txt_filament_stuck     = ""; // Empty here, set from the error description
#endif
#if HAS_MMU2()
// MMU-related
static constexpr const char *txt_mmu_engag_idler    = N_("Engaging idler");
static constexpr const char *txt_mmu_diseng_idler   = N_("Disengaging idler");
static constexpr const char *txt_mmu_unload_finda   = N_("Unloading to FINDA");
static constexpr const char *txt_mmu_unload_pulley  = N_("Unloading to pulley");
static constexpr const char *txt_mmu_feed_finda     = N_("Feeding to FINDA");
static constexpr const char *txt_mmu_feed_bondtech  = N_("Feeding to drive gear");
static constexpr const char *txt_mmu_feed_nozzle    = N_("Feeding to nozzle");
static constexpr const char *txt_mmu_avoid_grind    = N_("Avoiding grind");
static constexpr const char *txt_mmu_finish_moves   = N_("Finishing moves");
static constexpr const char *txt_mmu_err_internal   = N_("ERR Internal");
static constexpr const char *txt_mmu_err_help_fil   = N_("ERR Helping filament");
static constexpr const char *txt_mmu_err_tmc        = N_("ERR TMC failed");
static constexpr const char *txt_mmu_unload_filament= N_("Unloading filament");
static constexpr const char *txt_mmu_load_filament  = N_("Loading filament");
static constexpr const char *txt_mmu_select_slot    = N_("Selecting filament slot");
static constexpr const char *txt_mmu_prepare_blade  = N_("Preparing blade");
static constexpr const char *txt_mmu_push_filament  = N_("Pushing filament");
static constexpr const char *txt_mmu_perform_cut    = N_("Performing cut");
static constexpr const char *txt_mmu_return_selector= N_("Returning selector");
static constexpr const char *txt_mmu_park_selector  = N_("Parking selector");
static constexpr const char *txt_mmu_eject_filament = N_("Ejecting filament");
static constexpr const char *txt_mmu_retract_finda  = N_("Retracting from FINDA");
static constexpr const char *txt_mmu_homing         = N_("Homing");
static constexpr const char *txt_mmu_moving_selector= N_("Moving selector");
static constexpr const char *txt_mmu_feeding_fsensor= N_("Feeding to fsensor");
static constexpr const char *txt_mmu_hw_test_begin  = N_("HW test begin");
static constexpr const char *txt_mmu_hw_test_idler  = N_("HW test idler");
static constexpr const char *txt_mmu_hw_test_sel    = N_("HW test selector");
static constexpr const char *txt_mmu_hw_test_pulley = N_("HW test pulley");
static constexpr const char *txt_mmu_hw_test_cleanup= N_("HW test cleanup");
static constexpr const char *txt_mmu_hw_test_exec   = N_("HW test exec");
static constexpr const char *txt_mmu_hw_test_display= N_("HW test display");
static constexpr const char *txt_mmu_errhw_test_fail= N_("ERR HW test failed");
static constexpr const char *txt_mmu_insert_filament= N_("Press CONTINUE and push filament into MMU.");

//MMU_ErrWaitForUser, // need to distinguish error states based on prusa-error-codes @@TODO
static constexpr const char *txt_mmu_err_wait_user  = find_error(ErrCode::CONNECT_MMU_LOAD_UNLOAD_ERROR).err_text;
#endif

// function pointer for onEnter & onExit callbacks
using change_state_cb_t = void (*)();

struct State {
    constexpr State(const char *lbl, const PhaseResponses &btn_resp, const PhaseTexts &btn_labels, change_state_cb_t enter_cb = NULL, change_state_cb_t exit_cb = NULL)
        : label(lbl)
        , btn_resp(btn_resp)
        , btn_labels(btn_labels)
        , onEnter(enter_cb)
        , onExit(exit_cb) {}
    const char *label;
    const PhaseResponses &btn_resp;
    const PhaseTexts &btn_labels;
    // callbacks for phase start/end
    change_state_cb_t onEnter;
    change_state_cb_t onExit;
};

static constexpr State states[CountPhases<PhasesLoadUnload>()] = {
        { txt_first,                 ClientResponses::GetResponses(PhasesLoadUnload::initial),                       ph_txt_none },
        { txt_tool,                  ClientResponses::GetResponses(PhasesLoadUnload::ChangingTool),                  ph_txt_none },
        { txt_parking,               ClientResponses::GetResponses(PhasesLoadUnload::Parking_stoppable),             ph_txt_stop },
        { txt_parking,               ClientResponses::GetResponses(PhasesLoadUnload::Parking_unstoppable),           ph_txt_none },
        { txt_wait_temp,             ClientResponses::GetResponses(PhasesLoadUnload::WaitingTemp_stoppable),         ph_txt_stop },
        { txt_wait_temp,             ClientResponses::GetResponses(PhasesLoadUnload::WaitingTemp_unstoppable),       ph_txt_none },
        { txt_prep_ram,              ClientResponses::GetResponses(PhasesLoadUnload::PreparingToRam_stoppable),      ph_txt_stop },
        { txt_prep_ram,              ClientResponses::GetResponses(PhasesLoadUnload::PreparingToRam_unstoppable),    ph_txt_none },
        { txt_ram,                   ClientResponses::GetResponses(PhasesLoadUnload::Ramming_stoppable),             ph_txt_stop },
        { txt_ram,                   ClientResponses::GetResponses(PhasesLoadUnload::Ramming_unstoppable),           ph_txt_none },
        { txt_unload,               ClientResponses::GetResponses(PhasesLoadUnload::Unloading_stoppable),           ph_txt_stop },
        { txt_unload,               ClientResponses::GetResponses(PhasesLoadUnload::Unloading_unstoppable),         ph_txt_none },
        { txt_unload,               ClientResponses::GetResponses(PhasesLoadUnload::RemoveFilament),                ph_txt_stop },
        { txt_unload_confirm,       ClientResponses::GetResponses(PhasesLoadUnload::IsFilamentUnloaded),            ph_txt_yesno, DialogLoadUnload::phaseWaitSound },
        { txt_filament_not_in_fs,   ClientResponses::GetResponses(PhasesLoadUnload::FilamentNotInFS),               ph_txt_none, DialogLoadUnload::phaseAlertSound},
        { txt_manual_unload,        ClientResponses::GetResponses(PhasesLoadUnload::ManualUnload),                  ph_txt_continue, DialogLoadUnload::phaseStopSound },
        { txt_push_fil,             ClientResponses::GetResponses(PhasesLoadUnload::UserPush_stoppable),            ph_txt_continue_stop, DialogLoadUnload::phaseAlertSound },
        { txt_push_fil,             ClientResponses::GetResponses(PhasesLoadUnload::UserPush_unstoppable),          ph_txt_continue, DialogLoadUnload::phaseAlertSound },
        { txt_make_sure_inserted,   ClientResponses::GetResponses(PhasesLoadUnload::MakeSureInserted_stoppable),    ph_txt_stop, DialogLoadUnload::phaseAlertSound },
        { txt_make_sure_inserted,   ClientResponses::GetResponses(PhasesLoadUnload::MakeSureInserted_unstoppable),  ph_txt_none, DialogLoadUnload::phaseAlertSound },
        { txt_inserting,            ClientResponses::GetResponses(PhasesLoadUnload::Inserting_stoppable),           ph_txt_stop },
        { txt_inserting,            ClientResponses::GetResponses(PhasesLoadUnload::Inserting_unstoppable),         ph_txt_none },
        { txt_is_filament_in_gear,  ClientResponses::GetResponses(PhasesLoadUnload::IsFilamentInGear),              ph_txt_yesno },
        { txt_ejecting,             ClientResponses::GetResponses(PhasesLoadUnload::Ejecting_stoppable),            ph_txt_stop },
        { txt_ejecting,             ClientResponses::GetResponses(PhasesLoadUnload::Ejecting_unstoppable),          ph_txt_none },
        { txt_loading,              ClientResponses::GetResponses(PhasesLoadUnload::Loading_stoppable),             ph_txt_stop },
        { txt_loading,              ClientResponses::GetResponses(PhasesLoadUnload::Loading_unstoppable),           ph_txt_none },
        { txt_purging,              ClientResponses::GetResponses(PhasesLoadUnload::Purging_stoppable),             ph_txt_stop },
        { txt_purging,              ClientResponses::GetResponses(PhasesLoadUnload::Purging_unstoppable),           ph_txt_none },
        { txt_is_color,             ClientResponses::GetResponses(PhasesLoadUnload::IsColor),                       ph_txt_iscolor, DialogLoadUnload::phaseAlertSound },
        { txt_is_color,             ClientResponses::GetResponses(PhasesLoadUnload::IsColorPurge),                  ph_txt_iscolor_purge, DialogLoadUnload::phaseAlertSound },
        { txt_unparking,            ClientResponses::GetResponses(PhasesLoadUnload::Unparking),                     ph_txt_stop },
#if HAS_LOADCELL()
        { txt_filament_stuck,       ClientResponses::GetResponses(PhasesLoadUnload::FilamentStuck),                 ph_txt_unload, DialogLoadUnload::phaseAlertSound },
#endif
#if HAS_MMU2()
        { txt_mmu_insert_filament,  ClientResponses::GetResponses(PhasesLoadUnload::LoadFilamentIntoMMU),   ph_txt_none }, // TODO how the button is Continue
        { txt_mmu_engag_idler,      ClientResponses::GetResponses(PhasesLoadUnload::MMU_EngagingIdler),     ph_txt_none },
        { txt_mmu_diseng_idler,     ClientResponses::GetResponses(PhasesLoadUnload::MMU_DisengagingIdler),  ph_txt_none },
        { txt_mmu_unload_finda,     ClientResponses::GetResponses(PhasesLoadUnload::MMU_UnloadingToFinda),  ph_txt_none },
        { txt_mmu_unload_pulley,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_UnloadingToPulley), ph_txt_none },
        { txt_mmu_feed_finda,       ClientResponses::GetResponses(PhasesLoadUnload::MMU_FeedingToFinda),    ph_txt_none },
        { txt_mmu_feed_bondtech,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_FeedingToBondtech), ph_txt_none },
        { txt_mmu_feed_nozzle,      ClientResponses::GetResponses(PhasesLoadUnload::MMU_FeedingToNozzle),   ph_txt_none },
        { txt_mmu_avoid_grind,      ClientResponses::GetResponses(PhasesLoadUnload::MMU_AvoidingGrind),     ph_txt_none },
        { txt_mmu_finish_moves,     ClientResponses::GetResponses(PhasesLoadUnload::MMU_FinishingMoves),    ph_txt_none },
        { txt_mmu_diseng_idler,     ClientResponses::GetResponses(PhasesLoadUnload::MMU_ERRDisengagingIdler),ph_txt_none },
        { txt_mmu_engag_idler,      ClientResponses::GetResponses(PhasesLoadUnload::MMU_ERREngagingIdler),  ph_txt_none },

        // the one and only MMU Error screen
        { txt_mmu_err_wait_user,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_ERRWaitingForUser), ph_txt_none },

        { txt_mmu_err_internal,     ClientResponses::GetResponses(PhasesLoadUnload::MMU_ERRInternal),       ph_txt_none },
        { txt_mmu_err_help_fil,     ClientResponses::GetResponses(PhasesLoadUnload::MMU_ERRHelpingFilament),ph_txt_none },
        { txt_mmu_err_tmc,          ClientResponses::GetResponses(PhasesLoadUnload::MMU_ERRTMCFailed),      ph_txt_none },
        { txt_mmu_unload_filament,  ClientResponses::GetResponses(PhasesLoadUnload::MMU_UnloadingFilament), ph_txt_none },
        { txt_mmu_load_filament,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_LoadingFilament),   ph_txt_none },
        { txt_mmu_select_slot,      ClientResponses::GetResponses(PhasesLoadUnload::MMU_SelectingFilamentSlot),ph_txt_none },
        { txt_mmu_prepare_blade,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_PreparingBlade),    ph_txt_none },
        { txt_mmu_push_filament,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_PushingFilament),   ph_txt_none },
        { txt_mmu_perform_cut,      ClientResponses::GetResponses(PhasesLoadUnload::MMU_PerformingCut),     ph_txt_none },
        { txt_mmu_return_selector,  ClientResponses::GetResponses(PhasesLoadUnload::MMU_ReturningSelector), ph_txt_none },
        { txt_mmu_park_selector,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_ParkingSelector),   ph_txt_none },
        { txt_mmu_eject_filament,   ClientResponses::GetResponses(PhasesLoadUnload::MMU_EjectingFilament),  ph_txt_none },
        { txt_mmu_retract_finda,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_RetractingFromFinda),ph_txt_none },
        { txt_mmu_homing,           ClientResponses::GetResponses(PhasesLoadUnload::MMU_Homing),            ph_txt_none },
        { txt_mmu_moving_selector,  ClientResponses::GetResponses(PhasesLoadUnload::MMU_MovingSelector),    ph_txt_none },
        { txt_mmu_feeding_fsensor,  ClientResponses::GetResponses(PhasesLoadUnload::MMU_FeedingToFSensor),  ph_txt_none },
        { txt_mmu_hw_test_begin,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_HWTestBegin),       ph_txt_none },
        { txt_mmu_hw_test_idler,    ClientResponses::GetResponses(PhasesLoadUnload::MMU_HWTestIdler),       ph_txt_none },
        { txt_mmu_hw_test_sel,      ClientResponses::GetResponses(PhasesLoadUnload::MMU_HWTestSelector),    ph_txt_none },
        { txt_mmu_hw_test_pulley,   ClientResponses::GetResponses(PhasesLoadUnload::MMU_HWTestPulley),      ph_txt_none },
        { txt_mmu_hw_test_cleanup,  ClientResponses::GetResponses(PhasesLoadUnload::MMU_HWTestCleanup),     ph_txt_none },
        { txt_mmu_hw_test_exec,     ClientResponses::GetResponses(PhasesLoadUnload::MMU_HWTestExec),        ph_txt_none },
        { txt_mmu_hw_test_display,  ClientResponses::GetResponses(PhasesLoadUnload::MMU_HWTestDisplay),     ph_txt_none },
        { txt_mmu_errhw_test_fail,  ClientResponses::GetResponses(PhasesLoadUnload::MMU_ErrHwTestFailed),   ph_txt_none },
#endif
    };

static const State &get_current_state(PhasesLoadUnload current_phase) {
    return states[uint8_t(current_phase)];
}

// clang-format on
/*****************************************************************************/

static constexpr Rect16 notice_title_rect = { 86, 44, 374, 22 };
static constexpr Rect16 notice_text_rect = { 86, 72, 244, 140 };
static constexpr Rect16 notice_link_rect = { 86, 218, 244, 32 };
static constexpr Rect16 notice_icon_rect = { 370, 180, 59, 72 };
static constexpr Rect16 notice_icon_type_rect = { 24, 44, 48, 48 };
static constexpr Rect16 notice_qr_rect = { 350, 72, 100, 100 };
static constexpr const char *error_code_link_format = N_("More detail at\nprusa.io/%05u");
namespace {
constexpr size_t color_size { 16 };
constexpr size_t text_height { 21 };
constexpr size_t text_margin { 18 };
constexpr size_t top_of_bottom_part { GuiDefaults::ScreenHeight - GuiDefaults::FooterHeight - GuiDefaults::FramePadding - GuiDefaults::ButtonHeight - 5 };
constexpr Rect16 filament_color_icon_rect { 0, top_of_bottom_part - text_height + (text_height - color_size) / 2, color_size, color_size }; // x needs to be 0, to be set later
constexpr Rect16 filament_type_text_rect { text_margin, top_of_bottom_part - text_height, GuiDefaults::ScreenWidth - 2 * text_margin, 21 };
} // namespace

static const constexpr int PROGRESS_BAR_H = 16;
static const constexpr int PROGRESS_NUM_Y_OFFSET = 10;
static const constexpr int PROGRESS_BAR_TEXT_H = 30;
static const constexpr int PROGRESS_H = GuiDefaults::EnableDialogBigLayout ? 80 : (PROGRESS_BAR_H + PROGRESS_BAR_TEXT_H);
static const constexpr int LABEL_TEXT_PAD = 2;
static const constexpr int PROGRESS_BAR_CORNER_RADIUS = GuiDefaults::EnableDialogBigLayout ? 4 : 0;
static const constexpr int RADIO_BUTTON_H = GuiDefaults::ButtonHeight + GuiDefaults::FramePadding;
static const constexpr int TITLE_TOP = 70;
static const constexpr int PROGRESS_TOP = GuiDefaults::EnableDialogBigLayout ? 100 : 30;
static const constexpr int LABEL_TOP = GuiDefaults::EnableDialogBigLayout ? 180 : PROGRESS_TOP + PROGRESS_H;
static const constexpr int PROGRESS_BAR_X_PAD = GuiDefaults::EnableDialogBigLayout ? 24 : 10;

static Rect16 get_frame_rect(Rect16 rect) {
    return Rect16(
        rect.Left(),
        rect.Top(),
        rect.Width(),
        rect.Height() - GuiDefaults::FooterHeight);
}

static Rect16 get_title_rect(Rect16 rect) {
    return Rect16(rect.Left(), GuiDefaults::EnableDialogBigLayout ? TITLE_TOP : (int)rect.Top(), rect.Width(), 30);
}

static Rect16 get_progress_rect(Rect16 rect) {
    return Rect16(rect.Left() + PROGRESS_BAR_X_PAD, GuiDefaults::EnableDialogBigLayout ? PROGRESS_TOP : rect.Top() + PROGRESS_TOP, rect.Width() - 2 * PROGRESS_BAR_X_PAD, PROGRESS_H);
}

static Rect16 get_label_rect(Rect16 rect) {

    const int RADION_BUTTON_TOP = rect.Height() - RADIO_BUTTON_H - GuiDefaults::FooterHeight;
    const int LABEL_H = RADION_BUTTON_TOP - LABEL_TOP;
    return Rect16(rect.Left() + LABEL_TEXT_PAD, GuiDefaults::EnableDialogBigLayout ? LABEL_TOP : rect.Top() + LABEL_TOP, rect.Width() - 2 * LABEL_TEXT_PAD, LABEL_H);
}

static Rect16 get_progress_bar_rect(const Rect16 parent_rect) {
    const Rect16 rect = get_progress_rect(parent_rect);
    return {
        rect.Left(),
        rect.Top(),
        rect.Width(),
        PROGRESS_BAR_H
    };
}

static Rect16 get_progress_number_rect(const Rect16 parent_rect) {
    const Rect16 rect = get_progress_rect(parent_rect);
    return {
        rect.Left(),
        int16_t(rect.Top() + PROGRESS_BAR_H + PROGRESS_NUM_Y_OFFSET),
        rect.Width(),
        uint16_t(rect.Height() - PROGRESS_BAR_H - PROGRESS_NUM_Y_OFFSET)
    };
}

void DialogLoadUnload::set_progress_percent(uint8_t val) {
    if (val != progress_number.GetValue()) {
        progress_bar.SetProgressPercent(val);
        progress_number.SetValue(val);
    }
}

DialogLoadUnload::DialogLoadUnload(fsm::BaseData data)
    : IDialogMarlin(GuiDefaults::RectScreenNoHeader)
    , progress_frame(this, get_frame_rect(GetRect()))
    , title(&progress_frame, get_title_rect(GetRect()), is_multiline::no, is_closed_on_click_t::no, get_name(ProgressSerializerLoadUnload(data.GetData()).mode))
    , progress_bar(&progress_frame, get_progress_bar_rect(GetRect()), COLOR_ORANGE, GuiDefaults::EnableDialogBigLayout ? COLOR_DARK_GRAY : COLOR_GRAY, PROGRESS_BAR_CORNER_RADIUS)
    , progress_number(&progress_frame, get_progress_number_rect(GetRect()), 0, "%.0f%%", Font::big)
    , label(&progress_frame, get_label_rect(GetRect()), is_multiline::yes)
    , radio(&progress_frame, GuiDefaults::GetButtonRect_AvoidFooter(GetRect()), PhasesLoadUnload::initial)
    , footer(this
#if FOOTER_ITEMS_PER_LINE__ >= 5
          ,
          footer::Item::nozzle, footer::Item::bed, footer::Item::f_sensor
    #if HAS_MMU2()
          ,
          FSensors_instance().HasMMU() ? footer::Item::finda : footer::Item::none
    #elif HAS_SIDE_FSENSOR()
          ,
          footer::Item::f_sensor_side
    #else
          ,
          footer::Item::none
    #endif
#endif
          )
    , notice_frame(this, get_frame_rect(GetRect()))
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
    title.set_font(GuiDefaults::FontBig);
    title.SetAlignment(Align_t::Center());
    progress_number.SetAlignment(Align_t::Center());
    label.set_font(GuiDefaults::EnableDialogBigLayout ? Font::special : GuiDefaults::FontBig);
    label.SetAlignment(Align_t::CenterTop());

    progress_frame.CaptureNormalWindow(radio);
    CaptureNormalWindow(progress_frame);

    notice_frame.CaptureNormalWindow(notice_radio_button);

    notice_title.set_font(GuiDefaults::FontBig);

    notice_text.set_font(Font::special);

    filament_type_text.SetAlignment(Align_t::Center());
    filament_color_icon.SetRoundCorners();
    instance = this;

    notice_link.set_font(Font::small);

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

bool DialogLoadUnload::Change(fsm::BaseData base_data) {
    PhasesLoadUnload phase = GetEnumFromPhaseIndex<PhasesLoadUnload>(base_data.GetPhase());
    fsm::PhaseData data = base_data.GetData();
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
    #if HAS_MMU2()
        if (is_notice_mmu(phase)) {
            const MMU2::MMUErrDesc *ptr_desc = fsm::PointerSerializer<MMU2::MMUErrDesc>(data).Get();
            PhaseResponses responses {
                MMU2::ButtonOperationToResponse(ptr_desc->buttons[0]),
                MMU2::ButtonOperationToResponse(ptr_desc->buttons[1]),
                MMU2::ButtonOperationToResponse(ptr_desc->buttons[2])
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
            notice_update(ftrstd::to_underlying(err_desc.err_code), err_desc.err_title, err_desc.err_text, ErrType::WARNING);
        }
    #endif
        current_phase = phase;

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
    if ((!current_phase) || (current_phase != phase)) {
        phaseExit();
        current_phase = phase;
        phaseEnter();
    }

    set_progress_percent(deserialize_progress(data));
    return true;
}

void DialogLoadUnload::notice_update(uint16_t errCode, const char *errTitle, const char *errDesc, ErrType type) {
    switch (type) {
    case ErrType::ERROR:
        notice_icon_type.SetRes(&img::error_48x48);
        break;
    case ErrType::WARNING:
        notice_icon_type.SetRes(&img::warning_48x48);
        break;
    case ErrType::USER_ACTION:
        notice_icon_type.SetRes(&img::info_48x48);
        break;
    case ErrType::CONNECT:
        // We should not get an attention code in here at all, so just silence the compiler warning.
        break;
    }

    notice_title.SetText(_(errTitle));
    notice_text.SetText(_(errDesc));

    snprintf(error_code_str, sizeof(error_code_str), error_code_link_format, errCode);
    notice_link.SetText(string_view_utf8::MakeRAM((const uint8_t *)error_code_str));

    notice_qr.SetQRHeader(errCode);
}

string_view_utf8 DialogLoadUnload::get_name(LoadUnloadMode mode) {
    switch (mode) {
    case LoadUnloadMode::Change:
        return _("Changing filament");
    case LoadUnloadMode::Load:
        return _("Loading filament");
    case LoadUnloadMode::Unload:
        return _("Unloading filament");
    case LoadUnloadMode::FilamentStuck:
        return _("Reloading filament");
    case LoadUnloadMode::Purge:
        return _("Purging filament");
    case LoadUnloadMode::Test:
        return _("Testing filament");
    case LoadUnloadMode::Cut:
        return _("Cutting filament");
    case LoadUnloadMode::Eject:
        return _("Ejecting filament");
    }

    // In case we get some invalid data
    return string_view_utf8::MakeCPUFLASH((const uint8_t *)"Index error");
}

float DialogLoadUnload::deserialize_progress(fsm::PhaseData data) const {
    return ProgressSerializerLoadUnload(data).progress;
}

void DialogLoadUnload::phaseExit() {
    if (!current_phase) {
        return;
    }

    if (auto f = get_current_state(*current_phase).onExit) {
        f();
    }
}

void DialogLoadUnload::phaseEnter() {
    if (!current_phase) {
        return;
    }
    {
        radio.Change(*current_phase /*, states[phase].btn_resp, &states[phase].btn_labels*/); // TODO alternative button label support
        label.SetText(_(get_current_state(*current_phase).label));
        if (get_current_state(*current_phase).onEnter) {
            get_current_state(*current_phase).onEnter();
        }
    }

    const FilamentType filament_to_load = (mode == LoadUnloadMode::Load) ? filament::get_type_to_load() : FilamentType::none;
    const bool has_filament_to_load = (filament_to_load != FilamentType::none);
    const bool has_color_to_load = has_filament_to_load && filament::get_color_to_load().has_value();

    filament_type_text.set_visible(has_filament_to_load);
    filament_color_icon.set_visible(has_color_to_load);

    if (has_filament_to_load) {
        filament_type_parameters = filament::get_description(filament_to_load);
        filament_type_text.SetText(string_view_utf8::MakeRAM(filament_type_parameters.name));
    }

    if (has_color_to_load) {
        const int16_t left_pos = (GuiDefaults::ScreenWidth - (width(Font::normal) + 1) * (strlen(filament_type_parameters.name) + 1 + 1) - color_size) / 2; // make the pos to be on the left of the text (+ one added space to the left of the text, + additional one for some reason makes it work )
        const auto rect = filament_color_icon_rect + Rect16::X_t { static_cast<int16_t>(left_pos) };

        const auto col = filament::get_color_to_load().value();
        filament_color_icon.SetBackColor(col);
        filament_color_icon.SetRect(rect);
    }
}
