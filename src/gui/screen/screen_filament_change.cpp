#include "screen_filament_change.hpp"
#include "error_codes_mmu.hpp"
#include "filament_to_load.hpp"
#include "find_error.hpp"
#include "footer_line.hpp"
#include "fsm_loadunload_type.hpp"
#include "guiconfig/wizard_config.hpp"
#include "img_resources.hpp"
#include "mmu2/mmu2_error_converter.h"
#include "qr.hpp"
#include "sound.hpp"
#include "text_error_url.hpp"
#include "window_colored_rect.hpp"
#include "screen_fsm.hpp"
#include <gui/standard_frame/frame_prompt.hpp>

#include <option/has_mmu2.h>
#include <option/has_nozzle_cleaner.h>
#include <option/has_extruder_fsensor.h>
#include <bsod/bsod.h>

namespace {

static constexpr const char *txt_init = N_("Finishing buffered gcodes");
static constexpr const char *txt_tool = N_("Changing tool");
static constexpr const char *txt_parking = N_("Parking");
static constexpr const char *txt_unparking = N_("Unparking");
static constexpr const char *txt_wait_temp = N_("Waiting for temperature");
static constexpr const char *txt_ram = N_("Ramming");
static constexpr const char *txt_unload = N_("Unloading");
static constexpr const char *txt_unload_confirm = N_("Was filament unload successful?");
static constexpr const char *txt_filament_not_in_fs = N_("Please remove filament from filament sensor.");
#if HAS_INDX() // INDX has no idler
static constexpr const char *txt_manual_unload = N_("Manually remove the filament from the extruder.");
#else
static constexpr const char *txt_manual_unload = N_("Please open idler and remove filament manually");
#endif
static constexpr const char *txt_push_fil = N_("Push the filament into the extruder and then, while still pushing, press CONTINUE.");
static constexpr const char *txt_make_sure_inserted = N_("Make sure the filament is inserted through the sensor.");
static constexpr const char *txt_inserting = N_("Inserting");
static constexpr const char *txt_is_filament_in_gear = N_("Is filament in extruder gear?");
static constexpr const char *txt_ejecting = N_("Ejecting");
static constexpr const char *txt_loading = N_("Loading to nozzle");
static constexpr const char *txt_purging = N_("Purging");
#if HAS_INDX()
static constexpr const char *txt_is_color = N_("Was filament successfully extruded?");
#else
static constexpr const char *txt_is_color = N_("Is color correct?");
#endif
#if HAS_NOZZLE_CLEANER()
static constexpr const char *txt_nozzle_cleaning = N_("Cleaning nozzle");
#endif
#if HAS_AUTO_RETRACT()
static constexpr const char *txt_auto_retracting = N_("Auto-retracting filament");
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
static constexpr const char *txt_mmu_dummy_start = N_("");
#endif // HAS_MMU2()

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

static constexpr Rect16 notice_title_rect = { 86, 44, 374, 22 };
static constexpr Rect16 notice_text_rect = { 86, 72, 244, 140 };
static constexpr Rect16 notice_link_rect = { 86, 218, 244, 32 };
static constexpr Rect16 notice_icon_rect = { 370, 180, 59, 72 };
static constexpr Rect16 notice_icon_type_rect = { 24, 44, 48, 48 };
static constexpr Rect16 notice_qr_rect = { 350, 72, 100, 100 };

constexpr size_t color_size { 16 };
constexpr size_t text_height { 21 };
constexpr size_t text_margin { 18 };
constexpr size_t top_of_bottom_part { GuiDefaults::ScreenHeight - GuiDefaults::FooterHeight - GuiDefaults::FramePadding - GuiDefaults::ButtonHeight - 5 };
constexpr Rect16 filament_color_icon_rect { 0, top_of_bottom_part - text_height + (text_height - color_size) / 2, color_size, color_size }; // x needs to be 0, to be set later
constexpr Rect16 filament_type_text_rect { text_margin, top_of_bottom_part - text_height, GuiDefaults::ScreenWidth - 2 * text_margin, 21 };

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

const char *get_name(LoadUnloadMode mode) {
    switch (mode) {
    case LoadUnloadMode::Change:
        return N_("Changing filament");
    case LoadUnloadMode::Load:
        return N_("Loading filament");
    case LoadUnloadMode::Unload:
        return N_("Unloading filament");
    case LoadUnloadMode::FilamentStuck:
        return N_("Reloading filament");
    case LoadUnloadMode::Purge:
        return N_("Purging filament");
    case LoadUnloadMode::Test:
        return N_("Testing filament");
    case LoadUnloadMode::Cut:
        return N_("Cutting filament");
    case LoadUnloadMode::Eject:
        return N_("Ejecting filament");
    }
    bsod_unreachable();
}

template <typename Base>
class WithBeepAlertSound : public Base {

public:
    WithBeepAlertSound(auto &&...args)
        : Base(args...) {
        sound::play(SoundType::single_beep);
    }
};

template <typename Base>
class WithBeepWaitSound : public Base {

public:
    WithBeepWaitSound(auto &&...args)
        : Base(args...) {
        sound::play(SoundType::single_beep);
    }

    void update(const fsm::PhaseData &fsm_data) {
        Base::update(fsm_data);
        if (triggered_) {
            return;
        }
        triggered_ = true;

        const auto data = fsm::deserialize_data<FSMLoadUnloadData>(fsm_data);
        if (data.mode == LoadUnloadMode::Change || data.mode == LoadUnloadMode::FilamentStuck) { /// this sound should be beeping only for M600 || runout
            sound::play(SoundType::waiting_beep);
        }
    }

private:
    bool triggered_ = false;
};

using Phase = PhasesLoadUnload;

class FrameBase {
public:
    FrameBase(window_t *parent)
        : footer(parent, 0, footer::Item::nozzle, footer::Item::bed
#if HAS_EXTRUDER_FSENSOR()
            ,
            footer::Item::f_sensor
#endif
#if HAS_SIDE_FSENSOR()
            ,
            footer::Item::f_sensor_side
#endif
        ) {
    }
    ~FrameBase() {
        sound::stop();
    }

private:
    FooterLine footer;
};

class FrameProgress : public FrameBase {
public:
    FrameProgress(window_t *parent, Phase phase, const char *label_text)
        : FrameBase(parent)
        , title(parent, get_title_rect(parent->GetRect()), is_multiline::no, is_closed_on_click_t::no, {})
        , progress_bar(parent, get_progress_bar_rect(parent->GetRect()), COLOR_BRAND, GuiDefaults::EnableDialogBigLayout ? COLOR_DARK_GRAY : COLOR_GRAY, PROGRESS_BAR_CORNER_RADIUS)
        , progress_number(parent, get_progress_number_rect(parent->GetRect()), 0, "%.0f%%", Font::big)
        , label(parent, get_label_rect(parent->GetRect()), is_multiline::yes, is_closed_on_click_t::no, _(label_text))
        , radio(parent, WizardDefaults::RectRadioButton(1), phase)
        , filament_type_text(parent, filament_type_text_rect, is_multiline::no)
        , filament_color_icon(parent, filament_color_icon_rect) {

        title.set_font(GuiDefaults::FontBig);
        title.SetAlignment(Align_t::Center());
        progress_number.SetAlignment(Align_t::Center());
        label.set_font(GuiDefaults::EnableDialogBigLayout ? Font::special : GuiDefaults::FontBig);
        label.SetAlignment(Align_t::CenterTop());
        filament_type_text.SetAlignment(Align_t::Center());
        filament_color_icon.SetRoundCorners();

        static_cast<window_frame_t *>(parent)->CaptureNormalWindow(radio);
    }

    void update(fsm::PhaseData fsm_data) {
        auto deserialized_data = fsm::deserialize_data<FSMLoadUnloadData>(fsm_data);
        title.SetText(_(get_name(deserialized_data.mode)));
        progress_bar.set_progress_percent(deserialized_data.progress);
        progress_number.SetValue(deserialized_data.progress);

        if (!first_update_done) {
            first_update_done = true;
            first_update(deserialized_data);
        }
    }

private:
    void first_update(FSMLoadUnloadData fsm_data) {
        const FilamentType filament_to_load = (fsm_data.mode == LoadUnloadMode::Load) ? filament::get_type_to_load() : FilamentType::none;
        const bool has_filament_to_load = (filament_to_load != FilamentType::none);
        const bool has_color_to_load = has_filament_to_load && filament::get_color_to_load().has_value();
        filament_type_text.set_visible(has_filament_to_load);
        filament_color_icon.set_visible(has_color_to_load);

        if (has_filament_to_load) {
            filament_type_parameters = filament_to_load.parameters();
            filament_type_text.SetText(string_view_utf8::MakeRAM(filament_type_parameters.name.data()));
        }

        if (has_color_to_load) {
            const int16_t left_pos = (GuiDefaults::ScreenWidth - (width(Font::normal) + 1) * (filament_type_parameters.name.size() + 1 + 1) - color_size) / 2; // make the pos to be on the left of the text (+ one added space to the left of the text, + additional one for some reason makes it work )
            const auto rect = filament_color_icon_rect + Rect16::X_t { static_cast<int16_t>(left_pos) };

            const auto col = filament::get_color_to_load().value();
            filament_color_icon.SetBackColor(col);
            filament_color_icon.SetRect(rect);
        }
    }

    window_frame_t progress_frame;
    window_text_t title;
    WindowRoundedProgressBar progress_bar;
    window_numb_t progress_number;
    window_text_t label;
    RadioButtonFSM radio;

    window_text_t filament_type_text;
    window_colored_rect filament_color_icon;

    // Needs to be held in memory because we're rendering the name from it
    FilamentTypeParameters filament_type_parameters;

    bool first_update_done = false;
};

class FrameNotice : public FrameBase {
public:
    FrameNotice(window_t *parent, Phase phase)
        : FrameBase(parent)
        , notice_title(parent, GuiDefaults::MMUNoticeTitleRect, is_multiline::no)
        , notice_text(parent, GuiDefaults::MMUNoticeTextRect, is_multiline::yes)
        , notice_link(parent, notice_link_rect, ErrCode::ERR_UNDEF)
        , notice_icon_hand(parent, notice_icon_rect, &img::hand_qr_59x72)
        , notice_icon_type(parent, notice_icon_type_rect, &img::warning_48x48)
        , notice_qr(parent, notice_qr_rect, ErrCode::ERR_UNDEF)
        , radio_button(parent, GuiDefaults::GetButtonRect_AvoidFooter(parent->GetRect()), phase) {

        notice_title.set_font(GuiDefaults::FontBig);
        notice_text.set_font(Font::special);
        notice_link.set_font(Font::small);

        static_cast<window_frame_t *>(parent)->CaptureNormalWindow(radio_button);
    }

protected:
    void notice_update(uint16_t errCode, const char *errTitle, const char *errDesc, ErrType type) {
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

    window_text_t notice_title;
    window_text_t notice_text;
    TextErrorUrlWindow notice_link;
    window_icon_t notice_icon_hand;
    window_icon_t notice_icon_type;
    QRErrorUrlWindow notice_qr;
    RadioButtonFSM radio_button;
};
#if HAS_MMU2()
class FrameMMUNotice : public FrameNotice {
public:
    using FrameNotice::FrameNotice;

    void update(fsm::PhaseData fsm_data) {
        if (!first_update_done) {
            first_update_done = true;
            first_update(fsm_data);
        }
    }

private:
    void first_update(fsm::PhaseData fsm_data) {
        const auto *ptr_desc = fsm::deserialize_data<const MMU2::MMUErrDesc *>(fsm_data);
        PhaseResponses responses {
            MMU2::ButtonOperationToResponse(ptr_desc->buttons[0]),
            MMU2::ButtonOperationToResponse(ptr_desc->buttons[1]),
            MMU2::ButtonOperationToResponse(ptr_desc->buttons[2])
        };

        radio_button.set_fixed_width_buttons_count(3);
        radio_button.set_fsm_and_phase(Phase::MMU_ERRWaitingForUser, responses);
        notice_update(std::to_underlying(ptr_desc->err_code), ptr_desc->err_title, ptr_desc->err_text, ptr_desc->type);
    }

    bool first_update_done = false;
};
#endif

#if HAS_LOADCELL() && HAS_EXTRUDER_FSENSOR()
class FrameFStuckNotice : public FrameNotice {
public:
    FrameFStuckNotice(window_t *parent, Phase phase)
        : FrameNotice(parent, phase) {
        // There is no need for fsm_data in first update so it can be called from constructor
        first_update();
    }

private:
    void first_update() {
        auto err = find_error(ErrCode::ERR_MECHANICAL_STUCK_FILAMENT_DETECTED);

        radio_button.set_fixed_width_buttons_count(0);
        radio_button.set_fsm_and_phase(Phase::FilamentStuck);
        notice_update(std::to_underlying(err.err_code), err.err_title, err.err_text, err.type);
    }
};
#endif

#if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
class FrameLoadingObstructionNotice : public FrameNotice {
public:
    FrameLoadingObstructionNotice(window_t *parent, Phase phase)
        : FrameNotice(parent, phase) {
        // There is no need for fsm_data in first update so it can be called from constructor
        first_update(phase);
    }

private:
    void first_update(Phase phase) {
        auto err = find_error(ErrCode::ERR_MECHANICAL_LOADING_OBSTRUCTION);

        radio_button.set_fixed_width_buttons_count(2);
        radio_button.set_fsm_and_phase(phase, ClientResponses::get_available_responses(phase));
        notice_update(std::to_underlying(err.err_code), err.err_title, err.err_text, err.type);
    }
};
#endif
// -------------------------------FRAME STORAGE--------------------------------
using Frames = FrameDefinitionList<ScreenFilamentChange::FrameStorage,
    FrameDefinition<Phase::initial, FrameProgress, txt_init>,
    FrameDefinition<Phase::ChangingTool, FrameProgress, txt_tool>,
    FrameDefinition<Phase::Parking_stoppable, FrameProgress, txt_parking>,
    FrameDefinition<Phase::Parking_unstoppable, FrameProgress, txt_parking>,
    FrameDefinition<Phase::WaitingTemp_stoppable, FrameProgress, txt_wait_temp>,
    FrameDefinition<Phase::WaitingTemp_unstoppable, FrameProgress, txt_wait_temp>,
    FrameDefinition<Phase::Ramming_stoppable, FrameProgress, txt_ram>,
    FrameDefinition<Phase::Ramming_unstoppable, FrameProgress, txt_ram>,
    FrameDefinition<Phase::Unloading_stoppable, FrameProgress, txt_unload>,
    FrameDefinition<Phase::Unloading_unstoppable, FrameProgress, txt_unload>,
    FrameDefinition<Phase::IsFilamentUnloaded, WithBeepWaitSound<FrameProgress>, txt_unload_confirm>,
    FrameDefinition<Phase::FilamentNotInFS, WithBeepAlertSound<FrameProgress>, txt_filament_not_in_fs>,
    FrameDefinition<Phase::ManualUnload_continuable, FrameProgress, txt_manual_unload>,
    FrameDefinition<Phase::ManualUnload_uncontinuable, FrameProgress, txt_manual_unload>,
    FrameDefinition<Phase::UserPush_stoppable, WithBeepAlertSound<FrameProgress>, txt_push_fil>,
    FrameDefinition<Phase::UserPush_unstoppable, WithBeepAlertSound<FrameProgress>, txt_push_fil>,
    FrameDefinition<Phase::MakeSureInserted_stoppable, WithBeepAlertSound<FrameProgress>, txt_make_sure_inserted>,
    FrameDefinition<Phase::MakeSureInserted_unstoppable, WithBeepAlertSound<FrameProgress>, txt_make_sure_inserted>,
    FrameDefinition<Phase::Inserting_stoppable, FrameProgress, txt_inserting>,
    FrameDefinition<Phase::Inserting_unstoppable, FrameProgress, txt_inserting>,
    FrameDefinition<Phase::IsFilamentInGear, FrameProgress, txt_is_filament_in_gear>,
    FrameDefinition<Phase::Ejecting_stoppable, FrameProgress, txt_ejecting>,
    FrameDefinition<Phase::Ejecting_unstoppable, FrameProgress, txt_ejecting>,
#if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
    FrameDefinition<Phase::LoadingObstruction_stoppable, FrameLoadingObstructionNotice>,
    FrameDefinition<Phase::LoadingObstruction_unstoppable, FrameLoadingObstructionNotice>,
#endif
    FrameDefinition<Phase::Loading_stoppable, FrameProgress, txt_loading>,
    FrameDefinition<Phase::Loading_unstoppable, FrameProgress, txt_loading>,
    FrameDefinition<Phase::LoadingToGears_stoppable, FrameProgress, txt_inserting>,
    FrameDefinition<Phase::LoadingToGears_unstoppable, FrameProgress, txt_inserting>,
    FrameDefinition<Phase::Purging_stoppable, FrameProgress, txt_purging>,
    FrameDefinition<Phase::Purging_unstoppable, FrameProgress, txt_purging>,
    FrameDefinition<Phase::AwaitingFilament_stoppable, WithBeepAlertSound<FrameProgress>, txt_make_sure_inserted>,
    FrameDefinition<Phase::AwaitingFilament_unstoppable, WithBeepAlertSound<FrameProgress>, txt_make_sure_inserted>,
    FrameDefinition<Phase::IsColor, WithBeepAlertSound<FrameProgress>, txt_is_color>,
    FrameDefinition<Phase::IsColorPurge, WithBeepAlertSound<FrameProgress>, txt_is_color>,
#if HAS_NOZZLE_CLEANER()
    FrameDefinition<Phase::UnloadNozzleCleaning, FrameProgress, txt_nozzle_cleaning>,
    FrameDefinition<Phase::LoadNozzleCleaning, FrameProgress, txt_nozzle_cleaning>,
#endif
#if HAS_LOADCELL() && HAS_EXTRUDER_FSENSOR()
    FrameDefinition<Phase::FilamentStuck, FrameFStuckNotice>,
#endif
#if HAS_AUTO_RETRACT()
    FrameDefinition<Phase::AutoRetracting, FrameProgress, txt_auto_retracting>,
#endif
#if HAS_ANFC()
    FrameDefinition<Phase::OPT_UncommitedUsage, FramePrompt, N_("OpenPrintTag Pending Write"), N_("There is filament usage to be written to the OpenPrintTag. Make sure the spool is close to the NFC reader.")>,
#endif
#if HAS_INDX()
    FrameDefinition<Phase::FilamentCalibrationFailed, WithBeepAlertSound<FramePrompt>, N_("Filament Calibration Failed"), N_("Make sure the filament is properly inserted.")>,
#endif
#if HAS_MMU2()
    FrameDefinition<Phase::LoadFilamentIntoMMU, FrameProgress, txt_mmu_insert_filament>,
    FrameDefinition<Phase::MMUDummyStartNoAttention, FrameProgress, txt_mmu_dummy_start>,
    FrameDefinition<Phase::MMU_EngagingIdler, FrameProgress, txt_mmu_engag_idler>,
    FrameDefinition<Phase::MMU_DisengagingIdler, FrameProgress, txt_mmu_diseng_idler>,
    FrameDefinition<Phase::MMU_UnloadingToFinda, FrameProgress, txt_mmu_unload_finda>,
    FrameDefinition<Phase::MMU_UnloadingToPulley, FrameProgress, txt_mmu_unload_pulley>,
    FrameDefinition<Phase::MMU_FeedingToFinda, FrameProgress, txt_mmu_feed_finda>,
    FrameDefinition<Phase::MMU_FeedingToBondtech, FrameProgress, txt_mmu_feed_bondtech>,
    FrameDefinition<Phase::MMU_FeedingToNozzle, FrameProgress, txt_mmu_feed_nozzle>,
    FrameDefinition<Phase::MMU_AvoidingGrind, FrameProgress, txt_mmu_avoid_grind>,
    FrameDefinition<Phase::MMU_FinishingMoves, FrameProgress, txt_mmu_finish_moves>,
    FrameDefinition<Phase::MMU_ERRDisengagingIdler, FrameProgress, txt_mmu_diseng_idler>,
    FrameDefinition<Phase::MMU_ERREngagingIdler, FrameProgress, txt_mmu_engag_idler>,
    FrameDefinition<Phase::MMU_ERRWaitingForUser, FrameMMUNotice>,
    FrameDefinition<Phase::MMU_ERRInternal, FrameProgress, txt_mmu_err_internal>,
    FrameDefinition<Phase::MMU_ERRHelpingFilament, FrameProgress, txt_mmu_err_help_fil>,
    FrameDefinition<Phase::MMU_ERRTMCFailed, FrameProgress, txt_mmu_err_tmc>,
    FrameDefinition<Phase::MMU_UnloadingFilament, FrameProgress, txt_mmu_unload_filament>,
    FrameDefinition<Phase::MMU_LoadingFilament, FrameProgress, txt_mmu_load_filament>,
    FrameDefinition<Phase::MMU_SelectingFilamentSlot, FrameProgress, txt_mmu_select_slot>,
    FrameDefinition<Phase::MMU_PreparingBlade, FrameProgress, txt_mmu_prepare_blade>,
    FrameDefinition<Phase::MMU_PushingFilament, FrameProgress, txt_mmu_push_filament>,
    FrameDefinition<Phase::MMU_PerformingCut, FrameProgress, txt_mmu_perform_cut>,
    FrameDefinition<Phase::MMU_ReturningSelector, FrameProgress, txt_mmu_return_selector>,
    FrameDefinition<Phase::MMU_ParkingSelector, FrameProgress, txt_mmu_park_selector>,
    FrameDefinition<Phase::MMU_EjectingFilament, FrameProgress, txt_mmu_eject_filament>,
    FrameDefinition<Phase::MMU_RetractingFromFinda, FrameProgress, txt_mmu_retract_finda>,
    FrameDefinition<Phase::MMU_Homing, FrameProgress, txt_mmu_homing>,
    FrameDefinition<Phase::MMU_MovingSelector, FrameProgress, txt_mmu_moving_selector>,
    FrameDefinition<Phase::MMU_FeedingToFSensor, FrameProgress, txt_mmu_feeding_fsensor>,
    FrameDefinition<Phase::MMU_HWTestBegin, FrameProgress, txt_mmu_hw_test_begin>,
    FrameDefinition<Phase::MMU_HWTestIdler, FrameProgress, txt_mmu_hw_test_idler>,
    FrameDefinition<Phase::MMU_HWTestSelector, FrameProgress, txt_mmu_hw_test_sel>,
    FrameDefinition<Phase::MMU_HWTestPulley, FrameProgress, txt_mmu_hw_test_pulley>,
    FrameDefinition<Phase::MMU_HWTestCleanup, FrameProgress, txt_mmu_hw_test_cleanup>,
    FrameDefinition<Phase::MMU_HWTestExec, FrameProgress, txt_mmu_hw_test_exec>,
    FrameDefinition<Phase::MMU_HWTestDisplay, FrameProgress, txt_mmu_hw_test_display>,
    FrameDefinition<Phase::MMU_ErrHwTestFailed, FrameProgress, txt_mmu_errhw_test_fail>,
#endif
    FrameDefinition<Phase::Unparking, FrameProgress, txt_unparking>>;
} // anonymous namespace

ScreenFilamentChange::ScreenFilamentChange()
    : ScreenFSM(nullptr, GuiDefaults::RectScreenNoHeader) {
    create_frame();
}

ScreenFilamentChange::~ScreenFilamentChange() {
    sound::stop();
    destroy_frame();
}

void ScreenFilamentChange::create_frame() {
    const auto phase = get_phase();
    Frames::create_frame(frame_storage, phase, &inner_frame, phase);
}

void ScreenFilamentChange::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenFilamentChange::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}
