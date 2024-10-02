#include "DialogLoadUnload.hpp"

#include "gui.hpp" //resource_font
#include "sound.hpp"
#include "i18n.h"
#include "client_response_texts.hpp"
#include "ScreenHandler.hpp"
#include "fonts.hpp"
#include <mmu2/mmu2_error_converter.h>
#include "filament_sensors_handler.hpp"
#include "img_resources.hpp"
#include "fsm_loadunload_type.hpp"
#include <option/has_side_fsensor.h>
#include <option/has_mmu2.h>
#include <find_error.hpp>
#include <filament_to_load.hpp>
#include <common/enum_array.hpp>

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

static constexpr const char *txt_first = N_("Finishing buffered gcodes");
static constexpr const char *txt_tool = N_("Changing tool");
static constexpr const char *txt_parking = N_("Parking");
static constexpr const char *txt_unparking = N_("Unparking");
static constexpr const char *txt_wait_temp = N_("Waiting for temperature");
static constexpr const char *txt_prep_ram = N_("Preparing to ram");
static constexpr const char *txt_ram = N_("Ramming");
static constexpr const char *txt_unload = N_("Unloading");
static constexpr const char *txt_unload_confirm = N_("Was filament unload successful?");
static constexpr const char *txt_filament_not_in_fs = N_("Please remove filament from filament sensor.");
static constexpr const char *txt_manual_unload = N_("Please open idler and remove filament manually");
static constexpr const char *txt_push_fil = N_("Press CONTINUE and push filament into the extruder.");
static constexpr const char *txt_make_sure_inserted = N_("Make sure the filament is inserted through the sensor.");
static constexpr const char *txt_inserting = N_("Inserting");
static constexpr const char *txt_is_filament_in_gear = N_("Is filament in extruder gear?");
static constexpr const char *txt_ejecting = N_("Ejecting");
static constexpr const char *txt_loading = N_("Loading to nozzle");
static constexpr const char *txt_purging = N_("Purging");
static constexpr const char *txt_is_color = N_("Is color correct?");
#if HAS_LOADCELL()
static constexpr const char *txt_filament_stuck = ""; // Empty here, set from the error description
#endif
#if HAS_MMU2()
// MMU-related
static constexpr const char *txt_mmu_engag_idler = N_("Engaging idler");
static constexpr const char *txt_mmu_diseng_idler = N_("Disengaging idler");
static constexpr const char *txt_mmu_unload_finda = N_("Unloading to FINDA");
static constexpr const char *txt_mmu_unload_pulley = N_("Unloading to pulley");
static constexpr const char *txt_mmu_feed_finda = N_("Feeding to FINDA");
static constexpr const char *txt_mmu_feed_bondtech = N_("Feeding to drive gear");
static constexpr const char *txt_mmu_feed_nozzle = N_("Feeding to nozzle");
static constexpr const char *txt_mmu_avoid_grind = N_("Avoiding grind");
static constexpr const char *txt_mmu_finish_moves = N_("Finishing moves");
static constexpr const char *txt_mmu_err_internal = N_("ERR Internal");
static constexpr const char *txt_mmu_err_help_fil = N_("ERR Helping filament");
static constexpr const char *txt_mmu_err_tmc = N_("ERR TMC failed");
static constexpr const char *txt_mmu_unload_filament = N_("Unloading filament");
static constexpr const char *txt_mmu_load_filament = N_("Loading filament");
static constexpr const char *txt_mmu_select_slot = N_("Selecting filament slot");
static constexpr const char *txt_mmu_prepare_blade = N_("Preparing blade");
static constexpr const char *txt_mmu_push_filament = N_("Pushing filament");
static constexpr const char *txt_mmu_perform_cut = N_("Performing cut");
static constexpr const char *txt_mmu_return_selector = N_("Returning selector");
static constexpr const char *txt_mmu_park_selector = N_("Parking selector");
static constexpr const char *txt_mmu_eject_filament = N_("Ejecting filament");
static constexpr const char *txt_mmu_retract_finda = N_("Retracting from FINDA");
static constexpr const char *txt_mmu_homing = N_("Homing");
static constexpr const char *txt_mmu_moving_selector = N_("Moving selector");
static constexpr const char *txt_mmu_feeding_fsensor = N_("Feeding to fsensor");
static constexpr const char *txt_mmu_hw_test_begin = N_("HW test begin");
static constexpr const char *txt_mmu_hw_test_idler = N_("HW test idler");
static constexpr const char *txt_mmu_hw_test_sel = N_("HW test selector");
static constexpr const char *txt_mmu_hw_test_pulley = N_("HW test pulley");
static constexpr const char *txt_mmu_hw_test_cleanup = N_("HW test cleanup");
static constexpr const char *txt_mmu_hw_test_exec = N_("HW test exec");
static constexpr const char *txt_mmu_hw_test_display = N_("HW test display");
static constexpr const char *txt_mmu_errhw_test_fail = N_("ERR HW test failed");
static constexpr const char *txt_mmu_insert_filament = N_("Press CONTINUE and push filament into MMU.");

// MMU_ErrWaitForUser, // need to distinguish error states based on prusa-error-codes @@TODO
static constexpr const char *txt_mmu_err_wait_user = find_error(ErrCode::CONNECT_MMU_LOAD_UNLOAD_ERROR).err_text;
#endif

// function pointer for onEnter & onExit callbacks
using change_state_cb_t = void (*)();

struct State {
    const char *label;
    // callbacks for phase start/end
    change_state_cb_t onEnter = nullptr;
};

static constexpr EnumArray<PhasesLoadUnload, State, CountPhases<PhasesLoadUnload>()> states { {
    { PhasesLoadUnload::initial, { txt_first } },
    { PhasesLoadUnload::ChangingTool, { txt_tool } },
    { PhasesLoadUnload::Parking_stoppable, { txt_parking } },
    { PhasesLoadUnload::Parking_unstoppable, { txt_parking } },
    { PhasesLoadUnload::WaitingTemp_stoppable, { txt_wait_temp } },
    { PhasesLoadUnload::WaitingTemp_unstoppable, { txt_wait_temp } },
    { PhasesLoadUnload::PreparingToRam_stoppable, { txt_prep_ram } },
    { PhasesLoadUnload::PreparingToRam_unstoppable, { txt_prep_ram } },
    { PhasesLoadUnload::Ramming_stoppable, { txt_ram } },
    { PhasesLoadUnload::Ramming_unstoppable, { txt_ram } },
    { PhasesLoadUnload::Unloading_stoppable, { txt_unload } },
    { PhasesLoadUnload::Unloading_unstoppable, { txt_unload } },
    { PhasesLoadUnload::RemoveFilament, { txt_unload } },
    { PhasesLoadUnload::IsFilamentUnloaded, { txt_unload_confirm, DialogLoadUnload::phaseWaitSound } },
    { PhasesLoadUnload::FilamentNotInFS, { txt_filament_not_in_fs, DialogLoadUnload::phaseAlertSound } },
    { PhasesLoadUnload::ManualUnload, { txt_manual_unload, DialogLoadUnload::phaseStopSound } },
    { PhasesLoadUnload::UserPush_stoppable, { txt_push_fil, DialogLoadUnload::phaseAlertSound } },
    { PhasesLoadUnload::UserPush_unstoppable, { txt_push_fil, DialogLoadUnload::phaseAlertSound } },
    { PhasesLoadUnload::MakeSureInserted_stoppable, { txt_make_sure_inserted, DialogLoadUnload::phaseAlertSound } },
    { PhasesLoadUnload::MakeSureInserted_unstoppable, { txt_make_sure_inserted, DialogLoadUnload::phaseAlertSound } },
    { PhasesLoadUnload::Inserting_stoppable, { txt_inserting } },
    { PhasesLoadUnload::Inserting_unstoppable, { txt_inserting } },
    { PhasesLoadUnload::IsFilamentInGear, { txt_is_filament_in_gear } },
    { PhasesLoadUnload::Ejecting_stoppable, { txt_ejecting } },
    { PhasesLoadUnload::Ejecting_unstoppable, { txt_ejecting } },
    { PhasesLoadUnload::Loading_stoppable, { txt_loading } },
    { PhasesLoadUnload::Loading_unstoppable, { txt_loading } },
    { PhasesLoadUnload::Purging_stoppable, { txt_purging } },
    { PhasesLoadUnload::Purging_unstoppable, { txt_purging } },
    { PhasesLoadUnload::IsColor, { txt_is_color, DialogLoadUnload::phaseAlertSound } },
    { PhasesLoadUnload::IsColorPurge, { txt_is_color, DialogLoadUnload::phaseAlertSound } },
    { PhasesLoadUnload::Unparking, { txt_unparking } },
#if HAS_LOADCELL()
    { PhasesLoadUnload::FilamentStuck, { txt_filament_stuck, DialogLoadUnload::phaseAlertSound } },
#endif
#if HAS_MMU2()
    { PhasesLoadUnload::LoadFilamentIntoMMU, { txt_mmu_insert_filament } }, // TODO how the button is Continue
    { PhasesLoadUnload::MMU_EngagingIdler, { txt_mmu_engag_idler } },
    { PhasesLoadUnload::MMU_DisengagingIdler, { txt_mmu_diseng_idler } },
    { PhasesLoadUnload::MMU_UnloadingToFinda, { txt_mmu_unload_finda } },
    { PhasesLoadUnload::MMU_UnloadingToPulley, { txt_mmu_unload_pulley } },
    { PhasesLoadUnload::MMU_FeedingToFinda, { txt_mmu_feed_finda } },
    { PhasesLoadUnload::MMU_FeedingToBondtech, { txt_mmu_feed_bondtech } },
    { PhasesLoadUnload::MMU_FeedingToNozzle, { txt_mmu_feed_nozzle } },
    { PhasesLoadUnload::MMU_AvoidingGrind, { txt_mmu_avoid_grind } },
    { PhasesLoadUnload::MMU_FinishingMoves, { txt_mmu_finish_moves } },
    { PhasesLoadUnload::MMU_ERRDisengagingIdler, { txt_mmu_diseng_idler } },
    { PhasesLoadUnload::MMU_ERREngagingIdler, { txt_mmu_engag_idler } },

    // the one and only MMU Error screen
    { PhasesLoadUnload::MMU_ERRWaitingForUser, { txt_mmu_err_wait_user } },

    { PhasesLoadUnload::MMU_ERRInternal, { txt_mmu_err_internal } },
    { PhasesLoadUnload::MMU_ERRHelpingFilament, { txt_mmu_err_help_fil } },
    { PhasesLoadUnload::MMU_ERRTMCFailed, { txt_mmu_err_tmc } },
    { PhasesLoadUnload::MMU_UnloadingFilament, { txt_mmu_unload_filament } },
    { PhasesLoadUnload::MMU_LoadingFilament, { txt_mmu_load_filament } },
    { PhasesLoadUnload::MMU_SelectingFilamentSlot, { txt_mmu_select_slot } },
    { PhasesLoadUnload::MMU_PreparingBlade, { txt_mmu_prepare_blade } },
    { PhasesLoadUnload::MMU_PushingFilament, { txt_mmu_push_filament } },
    { PhasesLoadUnload::MMU_PerformingCut, { txt_mmu_perform_cut } },
    { PhasesLoadUnload::MMU_ReturningSelector, { txt_mmu_return_selector } },
    { PhasesLoadUnload::MMU_ParkingSelector, { txt_mmu_park_selector } },
    { PhasesLoadUnload::MMU_EjectingFilament, { txt_mmu_eject_filament } },
    { PhasesLoadUnload::MMU_RetractingFromFinda, { txt_mmu_retract_finda } },
    { PhasesLoadUnload::MMU_Homing, { txt_mmu_homing } },
    { PhasesLoadUnload::MMU_MovingSelector, { txt_mmu_moving_selector } },
    { PhasesLoadUnload::MMU_FeedingToFSensor, { txt_mmu_feeding_fsensor } },
    { PhasesLoadUnload::MMU_HWTestBegin, { txt_mmu_hw_test_begin } },
    { PhasesLoadUnload::MMU_HWTestIdler, { txt_mmu_hw_test_idler } },
    { PhasesLoadUnload::MMU_HWTestSelector, { txt_mmu_hw_test_sel } },
    { PhasesLoadUnload::MMU_HWTestPulley, { txt_mmu_hw_test_pulley } },
    { PhasesLoadUnload::MMU_HWTestCleanup, { txt_mmu_hw_test_cleanup } },
    { PhasesLoadUnload::MMU_HWTestExec, { txt_mmu_hw_test_exec } },
    { PhasesLoadUnload::MMU_HWTestDisplay, { txt_mmu_hw_test_display } },
    { PhasesLoadUnload::MMU_ErrHwTestFailed, { txt_mmu_errhw_test_fail } },
#endif
} };

static const State &get_current_state(PhasesLoadUnload current_phase) {
    return states[uint8_t(current_phase)];
}

static constexpr Rect16 notice_title_rect = { 86, 44, 374, 22 };
static constexpr Rect16 notice_text_rect = { 86, 72, 244, 140 };
static constexpr Rect16 notice_link_rect = { 86, 218, 244, 32 };
static constexpr Rect16 notice_icon_rect = { 370, 180, 59, 72 };
static constexpr Rect16 notice_icon_type_rect = { 24, 44, 48, 48 };
static constexpr Rect16 notice_qr_rect = { 350, 72, 100, 100 };
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
    , notice_link(&notice_frame, notice_link_rect, ErrCode::ERR_UNDEF)
    , notice_icon_hand(&notice_frame, notice_icon_rect, &img::hand_qr_59x72)
    , notice_icon_type(&notice_frame, notice_icon_type_rect, &img::warning_48x48)
    , notice_qr(&notice_frame, notice_qr_rect, ErrCode::ERR_UNDEF)
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
            const auto *ptr_desc = fsm::deserialize_data<const MMU2::MMUErrDesc *>(data);
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

    notice_link.set_error_code(ErrCode(errCode));

    notice_qr.set_error_code(ErrCode(errCode));
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
        filament_type_parameters = filament_to_load.parameters();
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
