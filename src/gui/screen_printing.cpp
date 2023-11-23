// screen_printing.cpp
#include "screen_printing.hpp"
#include "marlin_client.hpp"
#include "print_utils.hpp"
#include "ffconf.h"
#include "ScreenHandler.hpp"
#include <ctime>
#include "../lang/format_print_will_end.hpp"
#include "utility_extensions.hpp"
#include "window_dlg_popup.hpp"
#include "odometer.hpp"
#include "liveadjust_z.hpp"
#include "DialogMoveZ.hpp"
#include "metric.h"
#include "screen_menu_tune.hpp"
#include <option/has_human_interactions.h>
#include <option/has_loadcell.h>
#include <option/has_mmu2.h>
#include <option/has_toolchanger.h>
#if HAS_MMU2()
    #include <feature/prusa/MMU2/mmu2_mk4.h>
#endif

#ifdef DEBUG_FSENSOR_IN_HEADER
    #include "filament_sensors_handler.hpp"
#endif

#include "Marlin/src/module/motion.h"
#include "Marlin/src/feature/bed_preheat.hpp"

#if ENABLED(CRASH_RECOVERY)
    #include "../Marlin/src/feature/prusa/crash_recovery.hpp"
#endif

using namespace marlin_server;

void screen_printing_data_t::invalidate_print_state() {
    state__readonly__use_change_print_state = printing_state_t::COUNT;
}
printing_state_t screen_printing_data_t::GetState() const {
    return state__readonly__use_change_print_state;
}

void screen_printing_data_t::tuneAction() {
    if (buttons[ftrstd::to_underlying(BtnSocket::Left)].IsShadowed()) {
        return;
    }
    switch (GetState()) {
    case printing_state_t::PRINTING:
    case printing_state_t::ABSORBING_HEAT:
    case printing_state_t::PAUSED:
        Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuTune>);
        break;
    default:
        break;
    }
}

void screen_printing_data_t::pauseAction() {
    if (buttons[ftrstd::to_underlying(BtnSocket::Middle)].IsShadowed()) {
        return;
    }
    switch (GetState()) {
    case printing_state_t::PRINTING:
        marlin_client::print_pause();
        change_print_state();
        break;
    case printing_state_t::ABSORBING_HEAT:
        bed_preheat.skip_preheat();
        change_print_state();
        break;
    case printing_state_t::PAUSED:
        marlin_client::print_resume();
        change_print_state();
        break;
    case printing_state_t::STOPPED:
    case printing_state_t::PRINTED:
        screen_printing_reprint();
        change_print_state();
        break;
    default:
        break;
    }
}

void screen_printing_data_t::stopAction() {
    if (buttons[ftrstd::to_underlying(BtnSocket::Right)].IsShadowed()) {
        return;
    }
    switch (GetState()) {
    case printing_state_t::STOPPED:
    case printing_state_t::PRINTED:
        marlin_client::print_exit();
        return;
    case printing_state_t::PAUSING:
    case printing_state_t::RESUMING:
        return;
    default: {
        if (MsgBoxWarning(_("Are you sure to stop this printing?"), Responses_YesNo, 1)
            == Response::Yes) {
            stop_pressed = true;
            waiting_for_abort = true;
            marlin_client::print_abort();
            change_print_state();
        } else {
            return;
        }
    }
    }
}

/******************************************************************************/

namespace {
constexpr const char *txt_printing_time { N_("Printing time") };
constexpr const char *txt_print_started { N_("Print started") };
constexpr const char *txt_print_ended { N_("Print ended") };
constexpr const char *txt_consumed_material { N_("Consumed material") };
constexpr const char *txt_na { N_("N/A") };
#if PRINTER_IS_PRUSA_XL
constexpr const char *txt_wipe_tower_pretranslated { N_("Prime tower %dg") };
#else
constexpr const char *txt_wipe_tower_pretranslated { N_("Wipe tower %dg") };
#endif

constexpr auto end_result_font { IDR_FNT_SMALL };

constexpr size_t column_left { 30 };
constexpr size_t column_right { GuiDefaults::ScreenWidth / 2 + column_left };
constexpr size_t column_width { 240 - 2 * column_left };

constexpr size_t row_0 { 104 };
constexpr size_t row_height { 20 };

constexpr size_t get_row(size_t idx) {
    return row_0 + idx * row_height;
}

constexpr Rect16 printing_time_label_rect { column_left, get_row(0), column_width, row_height };
constexpr Rect16 printing_time_value_rect { column_left, get_row(1), column_width, row_height };

constexpr Rect16 print_started_label_rect { column_left, get_row(3), column_width, row_height };
constexpr Rect16 print_started_value_rect { column_left, get_row(4), column_width, row_height };

constexpr Rect16 print_ended_label_rect { column_left, get_row(6), column_width, row_height };
constexpr Rect16 print_ended_value_rect { column_left, get_row(7), column_width, row_height };

constexpr size_t consumed_material_row { 3 };

constexpr Rect16 consumed_material_label_rect { column_right, get_row(consumed_material_row), column_width, row_height };

constexpr Rect16 get_consumed_material_rect(size_t idx) {
    return Rect16 { column_right, static_cast<int16_t>(get_row(consumed_material_row + 1 + idx)), column_width, row_height };
}

constexpr Rect16 consumed_wipe_tower_value_rect { column_right, get_row(7), column_width, row_height }; // row is going to be set dynamically

template <size_t... Is>
std::array<window_text_t, sizeof...(Is)> make_consumed_material_values(std::index_sequence<Is...>, window_t *parent) {
    //  this is just fancy template way to init array in constructor initializer_list
    return { (window_text_t { parent, get_consumed_material_rect(Is), is_multiline::no })... };
}

#if defined(USE_ST7789)
constexpr auto etime_val_font { IDR_FNT_SMALL };
#elif defined(USE_ILI9488)
constexpr auto etime_val_font { IDR_FNT_NORMAL };

constexpr auto arrow_left_res { &img::arrow_left_10x16 };
constexpr auto arrow_right_res { &img::arrow_right_10x16 };

constexpr size_t middle_of_buttons { 185 + 40 };
constexpr Rect16 arrow_left_rect { column_left - arrow_left_res->w, middle_of_buttons - arrow_left_res->h / 2, arrow_left_res->h, arrow_left_res->w };
constexpr Rect16 arrow_right_rect { column_right + column_width, middle_of_buttons - arrow_right_res->h / 2, arrow_right_res->h, arrow_right_res->w };

constexpr size_t rotating_circles_height { 5 };
constexpr size_t rotating_circles_width { 35 };
constexpr size_t rotating_circles_left_offset { 0 };
constexpr Rect16 rotating_circles_rect { column_left + rotating_circles_left_offset, get_row(1) + resource_font_size(etime_val_font).h + 5, rotating_circles_width, rotating_circles_height };
#endif

} // namespace

screen_printing_data_t::screen_printing_data_t()
    : AddSuperWindow<ScreenPrintingModel>(_(caption))
#if (defined(USE_ILI9488))
    , print_progress(this)
    , printing_time_label(this, printing_time_label_rect, is_multiline::no, is_closed_on_click_t::no, _(txt_printing_time))
    , printing_time_value(this, printing_time_value_rect, is_multiline::no)

    , print_started_label(this, print_started_label_rect, is_multiline::no, is_closed_on_click_t::no, _(txt_print_started))
    , print_started_value(this, print_started_value_rect, is_multiline::no)

    , print_ended_label(this, print_ended_label_rect, is_multiline::no, is_closed_on_click_t::no, _(txt_print_ended))
    , print_ended_value(this, print_ended_value_rect, is_multiline::no)

    , consumed_material_label(this, consumed_material_label_rect, is_multiline::no, is_closed_on_click_t::no, _(txt_consumed_material))
    , consumed_material_values(make_consumed_material_values(std::make_index_sequence<EXTRUDERS>(), this))
    , consumed_wipe_tower_value(this, consumed_wipe_tower_value_rect, is_multiline::no)
    , arrow_left(this, arrow_left_rect, arrow_left_res)
    , arrow_right(this, arrow_right_rect, arrow_right_res)
    , rotating_circles(this, rotating_circles_rect, ftrstd::to_underlying(CurrentlyShowing::_count))
#endif
#if defined(USE_ST7789)
    , w_filename(this, Rect16(10, 33, 220, 29))
    , w_progress(this, Rect16(10, 70, GuiDefaults::RectScreen.Width() - 2 * 10, 16))
    , w_progress_txt(this, Rect16(10, 86, GuiDefaults::RectScreen.Width() - 2 * 10, 30)) // font: Normal (11x18 px)
    , w_time_label(this, Rect16(10, 128, 101, 20), is_multiline::no)
    , w_time_value(this, Rect16(10, 148, 101, 20), is_multiline::no)
    , w_etime_label(this, Rect16(130, 128, 101, 20), is_multiline::no)
    , w_etime_value(this, Rect16(120, 148, 111, 37), is_multiline::yes)
#elif defined(USE_ILI9488)
    , w_filename(this, Rect16(30, 38, 420, 24))
    , w_progress(this, Rect16(30, 65, GuiDefaults::RectScreen.Width() - 2 * 30, 16))
    , w_progress_txt(this, Rect16(300, get_row(0) - 2, 135, 54)) // Left side option: 30, 115, 100, 54 | font: Large (53x30 px)
    , w_etime_label(this, Rect16(30, get_row(0), 150, 20), is_multiline::no) // Right side option: 300, 118, 150, 20
    , w_etime_value(this, Rect16(30, get_row(1), 200, 23), is_multiline::no) // Right side option: 250, 138, 200, 23
#endif // USE_<display>
    , message_timer(0)
    , stop_pressed(false)
    , waiting_for_abort(false)
    , state__readonly__use_change_print_state(printing_state_t::COUNT)
#if defined(USE_ST7789)
    , popup_rect(Rect16::Merge(std::array<Rect16, 4>({ w_time_label.GetRect(), w_time_value.GetRect(), w_etime_label.GetRect(), w_etime_value.GetRect() })))
    , time_end_format(PT_t::init)
#elif defined(USE_ILI9488)
    , popup_rect(Rect16(30, get_row(0), 250, 70)) // Rect for printing messages from marlin.
#endif // USE_<display>
{
    marlin_client::error_clr(MARLIN_ERR_ProbingFailed);
    // we will handle HELD_RELEASED event in this window
    DisableLongHoldScreenAction();

    strlcpy(text_filament.data(), "999m", text_filament.size());

#if defined(USE_ST7789)
    // ST7789 specific adjustments
    Align_t align = Align_t::RightBottom();
    w_filename.SetAlignment(Align_t::LeftBottom());
    w_progress_txt.SetAlignment(Align_t::Center());
    w_etime_label.SetAlignment(Align_t::RightBottom());
    w_etime_value.SetAlignment(Align_t::RightTop());
    w_etime_value.SetPadding({ 0, 5, 0, 2 });

    w_progress_txt.set_font(resource_font(IDR_FNT_NORMAL));

    // ST7789 specific variable and it's label
    w_time_label.set_font(resource_font(IDR_FNT_SMALL));
    w_time_label.SetAlignment(align);
    w_time_label.SetPadding({ 0, 2, 0, 2 });
    w_time_label.SetText(_(txt_printing_time));

    w_time_value.set_font(resource_font(IDR_FNT_SMALL));
    w_time_value.SetAlignment(align);
    w_time_value.SetPadding({ 0, 2, 0, 2 });
#elif defined(USE_ILI9488)
    // ILI_9488 specific adjustments
    w_filename.SetAlignment(Align_t::LeftTop());
    w_progress_txt.SetAlignment(Align_t::RightTop());
    w_etime_label.SetAlignment(Align_t::LeftBottom());
    w_etime_value.SetAlignment(Align_t::LeftBottom());
    w_etime_value.SetPadding({ 0, 2, 0, 2 });

    w_etime_label.SetTextColor(COLOR_SILVER);
    w_progress_txt.set_font(resource_font(IDR_FNT_LARGE));
#endif // USE_<display>

    w_filename.set_font(resource_font(IDR_FNT_BIG));
    w_filename.SetPadding({ 0, 0, 0, 0 });
    // this MakeRAM is safe - vars->media_LFN is statically allocated (even though it may not be obvious at the first look)
    {
        // Update printed filename from marlin_server, sample LFN+SFN atomically
        auto lock = MarlinVarsLockGuard();
        marlin_vars()->media_LFN.copy_to(gui_media_LFN, sizeof(gui_media_LFN), lock);
        marlin_vars()->media_SFN_path.copy_to(gui_media_SFN_path, sizeof(gui_media_SFN_path), lock);
    }
    w_filename.SetText(string_view_utf8::MakeRAM((const uint8_t *)gui_media_LFN));

    w_etime_label.set_font(resource_font(IDR_FNT_SMALL));

#if defined(USE_ILI9488)
    print_progress.init_gcode_info();
#endif /*USE_ILI9488*/

    // Execute first print time update loop
    updateTimes();

    w_etime_value.set_font(resource_font(etime_val_font));

#if defined(USE_ILI9488)
    print_progress.Pause();
    last_e_axis_position = marlin_vars()->logical_curr_pos[MARLIN_VAR_INDEX_E];

    printing_time_label.SetTextColor(COLOR_SILVER);
    print_started_label.SetTextColor(COLOR_SILVER);
    print_ended_label.SetTextColor(COLOR_SILVER);
    consumed_material_label.SetTextColor(COLOR_SILVER);

    printing_time_label.set_font(resource_font(end_result_font));
    print_started_label.set_font(resource_font(end_result_font));
    print_ended_label.set_font(resource_font(end_result_font));
    consumed_material_label.set_font(resource_font(end_result_font));
    printing_time_value.set_font(resource_font(end_result_font));
    print_started_value.set_font(resource_font(end_result_font));
    print_ended_value.set_font(resource_font(end_result_font));
    for (auto &consumed_material_value : consumed_material_values) {
        consumed_material_value.set_font(resource_font(end_result_font));
    }
    consumed_wipe_tower_value.set_font(resource_font(end_result_font));
    rotating_circles.set_one_circle_mode(true);

    hide_end_result_fields();
    arrow_left.Hide();
#endif
}

#ifdef DEBUG_FSENSOR_IN_HEADER
extern int _is_in_M600_flg;
extern uint32_t *pCommand;
#endif

void screen_printing_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
#ifdef DEBUG_FSENSOR_IN_HEADER
    static int _last = 0;
    if (gui::GetTick() - _last > 300) {
        _last = gui::GetTick();

        static char buff[] = "Sx Mx x xxxx"; //"x"s are replaced
        buff[1] = FSensors_instance().Get() + '0'; // S0 init, S1 has filament, S2 no filament, S3 not connected, S4 disabled
        buff[4] = FSensors_instance().GetM600_send_on(); // Me edge, Ml level, Mn never, Mx undefined
        buff[6] = FSensors_instance().WasM600_send() ? 's' : 'n'; // s == send, n== not send
        buff[8] = _is_in_M600_flg ? 'M' : '0'; // M == marlin is doing M600
        buff[9] = marlin_event(Event::CommandBegin) ? 'B' : '0'; // B == Event begin
        buff[10] = marlin_command() == MARLIN_CMD_M600 ? 'C' : '0'; // C == Command M600
        buff[11] = *pCommand == MARLIN_CMD_M600 ? 's' : '0'; // s == server - Command M600
        header.SetText(buff);
    }

#endif

    /// check stop clicked when MBL is running
    printing_state_t p_state = GetState();
    if (
        stop_pressed
        && waiting_for_abort
        && marlin_client::get_command() != Cmd::G29
        && (p_state == printing_state_t::ABORTING || p_state == printing_state_t::PAUSED)) {
        marlin_client::print_abort();
        waiting_for_abort = false;
        return;
    }

#if HAS_LOADCELL() && ENABLED(PROBE_CLEANUP_SUPPORT)
    if ((p_state == printing_state_t::PRINTED || p_state == printing_state_t::PAUSED) && marlin_client::error(MARLIN_ERR_NozzleCleaningFailed)) {
        marlin_client::error_clr(MARLIN_ERR_NozzleCleaningFailed);
        if (MsgBox(_("Nozzle cleaning failed."), Responses_RetryAbort) == Response::Retry) {
            marlin_client::print_resume();
        } else {
            marlin_client::print_abort();
            return;
        }
    }
#endif

#if HAS_BED_PROBE
    if ((p_state == printing_state_t::PRINTED || p_state == printing_state_t::PAUSED) && marlin_client::error(MARLIN_ERR_ProbingFailed)) {
        marlin_client::error_clr(MARLIN_ERR_ProbingFailed);
        if (MsgBox(_("Bed leveling failed. Try again?"), Responses_YesNo) == Response::Yes) {
            marlin_client::print_resume();
        } else {
            marlin_client::print_abort();
            return;
        }
    }
#endif

    change_print_state();

    /// -- Print time update loop
    updateTimes();

    /// -- close screen when print is done / stopped and USB media is removed
    if (!marlin_vars()->media_inserted && (p_state == printing_state_t::PRINTED || p_state == printing_state_t::STOPPED)) {
        marlin_client::print_exit();
        return;
    }

    /// -- check when media is or isn't inserted
    if (event == GUI_event_t::MEDIA) {
        /// -- check for enable/disable resume button
        set_pause_icon_and_label();
    }
    if (event == GUI_event_t::HELD_RELEASED) {
        if (marlin_vars()->logical_curr_pos[2 /* Z Axis */] <= 1.0f && p_state == printing_state_t::PRINTING) {
            LiveAdjustZ::Show();
        } else if (p_state == printing_state_t::PRINTED || p_state == printing_state_t::STOPPED) {
            DialogMoveZ::Show();
        }
        return;
    }
#if defined(USE_ILI9488)
    if (event == GUI_event_t::LOOP && p_state == printing_state_t::PRINTING) {
        auto vars = marlin_vars();
        const bool midprint = vars->logical_curr_pos[MARLIN_VAR_INDEX_Z] >= 1.0f;
        const bool extruder_moved = (vars->logical_curr_pos[MARLIN_VAR_INDEX_E] - last_e_axis_position) > 0
            && vars->logical_curr_pos[MARLIN_VAR_INDEX_E] > 0
            && last_e_axis_position > 0; // Ignore negative movements and reset of E position (e.g. retraction)
        if (print_progress.isPaused() && midprint && extruder_moved) {
            print_progress.Resume();
        } else if (print_progress.isPaused()) {
            last_e_axis_position = vars->logical_curr_pos[MARLIN_VAR_INDEX_E];
        }
    }
#endif

    if (p_state == printing_state_t::PRINTED || p_state == printing_state_t::STOPPED) {
#if defined(USE_ILI9488)
        if (p_state == printing_state_t::PRINTED) {
            print_progress.Pause();
        } else {
            print_progress.StoppedMode();
        }
#endif
        hide_time_information();
    } else {
#if defined(USE_ILI9488)
        print_progress.PrintingMode();
#endif
        show_time_information();
    }

#if defined(USE_ILI9488)
    if (shown_end_result && event == GUI_event_t::ENC_DN
        && ((buttons[0].IsEnabled() && buttons[0].IsFocused()) || (!buttons[0].IsEnabled() && buttons[1].IsFocused()))) {
        start_showing_end_result();
        return;
    }

    if (p_state == printing_state_t::PRINTED && !shown_end_result) {
        start_showing_end_result();
        return;
    }

    if (showing_end_result && (event == GUI_event_t::ENC_UP)) {
        stop_showing_end_result();
        return;
    }

    if (!showing_end_result) {
        SuperWindowEvent(sender, event, param);
    }
#else
    SuperWindowEvent(sender, event, param);
#endif
}

#if defined(USE_ILI9488)
void screen_printing_data_t::start_showing_end_result() {

    // hide previous
    for (auto &button : buttons) {
        button.Hide();
    }

    for (auto &label : labels) {
        label.Hide();
    }

    arrow_left.Hide();

    hide_time_information(); // OK because currently we never show remaining time at the end

    // show end result

    auto &gcode { GCodeInfo::getInstance() };

    if (gcode.get_printing_time()[0]) {
        snprintf(printing_time_value_buffer.data(), printing_time_value_buffer.size(), "%s", gcode.get_printing_time().data());
    } else {
        snprintf(printing_time_value_buffer.data(), printing_time_value_buffer.size(), "unknown");
    }

    printing_time_label.Show();
    printing_time_value.Show();
    printing_time_value.SetText(_(printing_time_value_buffer.data()));

    print_started_label.Show();
    print_started_value.Show();
    print_ended_label.Show();
    print_ended_value.Show();

    {
        auto print_one = [time_format = time_tools::get_time_format()](MarlinVariableLocked<time_t> &time_holder, DateBufferT &buffer, window_text_t &text_value) {
            struct tm print_tm;
            time_holder.execute_with([&](const time_t &print_time) {
                localtime_r(&print_time, &print_tm);
            });

            print_tm.tm_hour += config_store().timezone.get();

            const time_t adjusted_print_time = mktime(&print_tm);
            localtime_r(&adjusted_print_time, &print_tm);

            FormatMsgPrintWillEnd::Date(buffer.data(), buffer.size(), &print_tm, time_format == time_tools::TimeFormat::_24h, FormatMsgPrintWillEnd::ISO);

            text_value.SetText(_(buffer.data()));
        };

        print_one(marlin_vars()->print_start_time, print_started_value_buffer, print_started_value);
        print_one(marlin_vars()->print_end_time, print_ended_value_buffer, print_ended_value);
    }

    static constexpr float minimum_grams_valid { 1.0f };
    const size_t num_extruders_with_valid_grams {
        [&]() {
            size_t found = 0;
            for (size_t i = 0; i < EXTRUDERS; ++i) {

                const auto &ext_info { gcode.get_extruder_info(i) };
                if (ext_info.used() && ext_info.filament_used_g.has_value() && ext_info.filament_used_g.value() >= minimum_grams_valid) {
                    ++found;
                }
            }

            return found;
        }()
    }; // holds how many extruders were printing with specified grammage that's big enough to show
    static constexpr auto keep_progress_threshold { 2 }; // anymore used gcodes will trigger hiding progress text and moving right column to the top;

    const bool have_valid_wipe_tower_grams {
        [&]() {
    #if EXTRUDERS > 1
            return gcode.get_filament_wipe_tower_g().has_value() && gcode.get_filament_wipe_tower_g().value() >= minimum_grams_valid;
    #else
            return false;
    #endif
        }()
    };

    // setup fields
    if (num_extruders_with_valid_grams > keep_progress_threshold) { // and have wipe tower grams?
        // prepare right column to be without progress_txt
        w_progress_txt.Hide();

        auto place_one = [](window_text_t &text_field, Rect16 default_rect, Rect16::Top_t new_top) {
            default_rect.set(new_top);
            text_field.SetRect(default_rect);
        };

        place_one(consumed_material_label, consumed_material_label_rect, get_row(0));

        for (size_t i = 0; i < num_extruders_with_valid_grams; ++i) {
            place_one(consumed_material_values[i], get_consumed_material_rect(i), get_row(i + 1));
        }

        place_one(consumed_wipe_tower_value, consumed_wipe_tower_value_rect, get_row(num_extruders_with_valid_grams + 2));
    } else {
        // Right column contains progress_txt, so reset places

        w_progress_txt.Show(); // make sure it's shown

        consumed_material_label.SetRect(consumed_material_label_rect);
        for (size_t i = num_extruders_with_valid_grams; i < std::size(consumed_material_values); ++i) {
            consumed_material_values[i].SetRect(get_consumed_material_rect(i));
        }

        // show wipe tower info
        if (have_valid_wipe_tower_grams) {

            Rect16 tmp = consumed_wipe_tower_value_rect;

            if (num_extruders_with_valid_grams > 0) {
                // print after individual tools with a blank space in between
                tmp.set(Rect16::Top_t { static_cast<int16_t>(get_row(consumed_material_row + num_extruders_with_valid_grams + 2)) });

            } else {
                // don't do blank space if we're only showing wipe tower
                tmp.set(Rect16::Top_t { static_cast<int16_t>(get_row(consumed_material_row + 1)) });
            }

            consumed_wipe_tower_value.SetRect(tmp);
        }
    }

    consumed_material_label.Show();
    if (num_extruders_with_valid_grams > 0) {
        for (size_t i = 0, consumed_material_line_idx = 0; i < EXTRUDERS; ++i) {
            const auto &ext_info { gcode.get_extruder_info(i) };
            if (!ext_info.used() || !ext_info.filament_used_g.has_value() || ext_info.filament_used_g.value() < minimum_grams_valid) {
                continue;
            }

            const auto &fname { ext_info.filament_name };
            const auto &used_g { ext_info.filament_used_g.value() }; // guaranteed to have value, see above guard

            auto print_fname = [&]() {
                return fname.has_value() ? fname.value().data() : "---";
            };

            auto &buff { consumed_material_values_buffers[consumed_material_line_idx] };

            const bool show_t_label {
    #if EXTRUDERS > 1
                []() {
        #if HAS_MMU2()
                    if (MMU2::mmu2.Enabled()) {
                        return true;
                    }
        #endif
        #if HAS_TOOLCHANGER()
                    if (prusa_toolchanger.is_toolchanger_enabled()) {
                        return true;
                    }
        #endif
                    return false;
                }()

    #else
                false
    #endif
            };

            if (show_t_label) {
                snprintf(buff.data(), buff.size(), "T%d %s %dg", i + 1, print_fname(), static_cast<int>(used_g));
            } else {
                snprintf(buff.data(), buff.size(), "%s %dg", print_fname(), static_cast<int>(used_g));
            }

            consumed_material_values[consumed_material_line_idx].SetText(_(buff.data()));
            consumed_material_values[consumed_material_line_idx].Show();
            ++consumed_material_line_idx;
        }
    }

    #if EXTRUDERS > 1
    // wipe tower
    if (have_valid_wipe_tower_grams) {

        auto &buff { consumed_wipe_tower_value_buffer };

        char translated_fmt[buff.size()];
        _(txt_wipe_tower_pretranslated).copyToRAM(translated_fmt, sizeof(translated_fmt));
        snprintf(buff.data(), buff.size(), translated_fmt, static_cast<int>(gcode.get_filament_wipe_tower_g().value()));

        consumed_wipe_tower_value.SetText(_(buff.data()));
        consumed_wipe_tower_value.Show();
    }
    #endif

    if (num_extruders_with_valid_grams == 0 && !have_valid_wipe_tower_grams) {

        auto &buff { consumed_material_values_buffers[0] };
        snprintf(buff.data(), buff.size(), "---");
        consumed_material_values[0].SetText(_(buff.data()));
        consumed_material_values[0].Show();
    }

    arrow_right.Show();

    showing_end_result = true;
    shown_end_result = true;
}

void screen_printing_data_t::stop_showing_end_result() {
    // show previous
    for (auto &button : buttons) {
        button.Show();
    }

    for (auto &label : labels) {
        label.Show();
    }

    w_progress_txt.Show(); // make sure it's shown, end result might hide it

    hide_end_result_fields();
    arrow_left.Show();

    showing_end_result = false;
}

void screen_printing_data_t::hide_end_result_fields() {
    printing_time_label.Hide();
    printing_time_value.Hide();

    print_started_label.Hide();
    print_started_value.Hide();

    print_ended_label.Hide();
    print_ended_value.Hide();

    consumed_material_label.Hide();
    for (auto &consumed_material_value : consumed_material_values) {
        consumed_material_value.Hide();
    }

    consumed_wipe_tower_value.Hide();
    arrow_right.Hide();
}
#endif

void screen_printing_data_t::show_time_information() {
    w_etime_label.Show();
    w_etime_value.Show();

#if defined(USE_ILI9488)
    rotating_circles.Show();
#endif
    updateTimes(); // make sure the data is valid
}

void screen_printing_data_t::hide_time_information() {
    w_etime_label.Hide();
    w_etime_value.Hide();

#if defined(USE_ILI9488)
    rotating_circles.Hide();
#endif
}

void screen_printing_data_t::updateTimes() {
#if defined(USE_ST7789)
    PT_t time_format = print_time.update_loop(time_end_format, &w_etime_value, &w_time_value);

    if (time_format != time_end_format) {
        switch (time_format) {
        case PT_t::init: // should not happen
            return;
        case PT_t::countdown:
            w_etime_label.SetText(_(PrintTime::EN_STR_COUNTDOWN));
            break;
        case PT_t::timestamp:
            w_etime_label.SetText(_(PrintTime::EN_STR_TIMESTAMP));
            break;
        }

        time_end_format = time_format;
    }
#elif defined(USE_ILI9488)

    if (!w_etime_value.HasVisibleFlag() || !w_etime_label.HasVisibleFlag()) {
        return;
    }

    if (auto now = ticks_s(); now - last_update_time_s > rotation_time_s) {
        // do rotation

        currently_showing = static_cast<CurrentlyShowing>(ftrstd::to_underlying(currently_showing) + 1 == ftrstd::to_underlying(CurrentlyShowing::_count) ? 0 : ftrstd::to_underlying(currently_showing) + 1);

        rotating_circles.set_index(ftrstd::to_underlying(currently_showing));

        last_update_time_s = now;
    }

    auto time_to_end = marlin_vars()->time_to_end.get();
    if ((currently_showing == CurrentlyShowing::end_time
            || currently_showing == CurrentlyShowing::remaining_time)
        && (time_to_end == marlin_server::TIME_TO_END_INVALID || time_to_end > 60 * 60 * 24 * 365)) {
        // invalidate for states that show time_to_end in some form if the time is invalid
        w_etime_value.SetText(_(txt_na));
        w_etime_value.SetTextColor(GuiDefaults::COLOR_VALUE_INVALID);
        return;
    }

    switch (currently_showing) {
    case CurrentlyShowing::end_time:
        w_etime_label.SetText(_(PrintTime::EN_STR_TIMESTAMP));

        if (!PrintTime::print_end_time(time_to_end, w_etime_value_buffer)) {
            w_etime_value.SetText(_(txt_na));
            w_etime_value.SetTextColor(GuiDefaults::COLOR_VALUE_INVALID);
            return;
        }
        break;
    case CurrentlyShowing::remaining_time:
        w_etime_label.SetText(_(PrintTime::EN_STR_COUNTDOWN));

        PrintTime::print_formatted_duration(time_to_end, w_etime_value_buffer);
        break;
    case CurrentlyShowing::time_since_start:
        w_etime_label.SetText(_(txt_printing_time));

        PrintTime::print_formatted_duration(marlin_vars()->print_duration.get(), w_etime_value_buffer, true);
        break;
    // Currently disabled, left in the code to ease re-enabling it
    // case CurrentlyShowing::time_to_change:
    //     w_etime_label.SetText(_("Next change in"));

    //     w_etime_value.SetText(_(txt_na));
    //     w_etime_value.SetTextColor(GuiDefaults::COLOR_VALUE_INVALID);
    //     return;
    case CurrentlyShowing::_count:
        assert(false); // invalid value, should never happen
        break;
    }

    // Add unknown marker
    if (marlin_vars()->print_speed != 100) {
        strlcat(w_etime_value_buffer.data(), "?", w_etime_value_buffer.size());
    }

    w_etime_value.SetText(_(w_etime_value_buffer.data()));
    w_etime_value.SetTextColor(GuiDefaults::COLOR_VALUE_VALID);
    w_etime_value.Invalidate(); // just to make sure

#endif // USE_ST7789
}

void screen_printing_data_t::screen_printing_reprint() {
    print_begin(gui_media_SFN_path, marlin_server::PreviewSkipIfAble::preview);
    screen_printing_data_t::updateTimes(); // reinit, but should be already set correctly
    SetButtonIconAndLabel(BtnSocket::Middle, BtnRes::Stop, LabelRes::Stop);

#ifndef DEBUG_FSENSOR_IN_HEADER
    header.SetText(_(caption));
#endif
}

// todo use it
/*static void mesh_err_stop_print() {
    float target_nozzle = marlin_vars()->target_nozzle;
    float target_bed = marlin_vars()->target_bed;
    marlin_print_abort();
    while (marlin_vars()->sd_printing) {
        gui_loop();
    }
    //marlin_park_head();
    marlin_client::gcode_printf("M104 S%F", (double)target_nozzle);
    marlin_client::gcode_printf("M140 S%F", (double)target_bed);
    marlin_client::gcode("G0 Z30"); //Z 30mm
    marlin_client::gcode("M84");    //Disable steppers
    while (marlin_vars()->pqueue) {
        gui_loop();
    }
}*/

void screen_printing_data_t::set_pause_icon_and_label() {
    // todo it is static, because menu tune is not dialog
    // switch (state__readonly__use_change_print_state)
    switch (GetState()) {
    case printing_state_t::COUNT:
    case printing_state_t::INITIAL:
    case printing_state_t::PRINTING:
    case printing_state_t::MBL_FAILED:
        EnableButton(BtnSocket::Middle);
        SetButtonIconAndLabel(BtnSocket::Middle, BtnRes::Pause, LabelRes::Pause);
        break;
    case printing_state_t::ABSORBING_HEAT:
        EnableButton(BtnSocket::Middle);
        SetButtonIconAndLabel(BtnSocket::Middle, BtnRes::Resume, LabelRes::Skip);
        break;
    case printing_state_t::PAUSING:
        DisableButton(BtnSocket::Middle);
        SetButtonIconAndLabel(BtnSocket::Middle, BtnRes::Pause, LabelRes::Pausing);
        break;
    case printing_state_t::PAUSED:
        EnableButton(BtnSocket::Middle);
        SetButtonIconAndLabel(BtnSocket::Middle, BtnRes::Resume, LabelRes::Resume);
        if (!marlin_vars()->media_inserted) {
            DisableButton(BtnSocket::Middle);
        }
        break;
    case printing_state_t::RESUMING:
        DisableButton(BtnSocket::Middle);
        SetButtonIconAndLabel(BtnSocket::Middle, BtnRes::Resume, LabelRes::Resuming);
        break;
    case printing_state_t::REHEATING:
    case printing_state_t::REHEATING_DONE:
        DisableButton(BtnSocket::Middle);
        SetButtonIconAndLabel(BtnSocket::Middle, BtnRes::Resume, LabelRes::Reheating);
        break;
    case printing_state_t::STOPPED:
    case printing_state_t::PRINTED:
        EnableButton(BtnSocket::Middle);
        SetButtonIconAndLabel(BtnSocket::Middle, BtnRes::Reprint, LabelRes::Reprint);
        break;
    case printing_state_t::ABORTING:
        DisableButton(BtnSocket::Middle);
        break;
    }

    switch (GetState()) {
    case printing_state_t::PAUSING:
        header.SetText(_("PAUSING ..."));
        break;
    case printing_state_t::MBL_FAILED:
    case printing_state_t::PAUSED:
        header.SetText(_("PAUSED"));
        break;
    case printing_state_t::ABORTING:
        header.SetText(_("ABORTING ..."));
        break;
    case printing_state_t::STOPPED:
        header.SetText(_("STOPPED"));
        break;
    case printing_state_t::PRINTED:
        header.SetText(_("FINISHED"));
        break;
    default: // else printing
        header.SetText(_(caption));
        break;
    }
}

void screen_printing_data_t::set_tune_icon_and_label() {
    SetButtonIconAndLabel(BtnSocket::Left, BtnRes::Settings, LabelRes::Settings);

    switch (GetState()) {
    case printing_state_t::PRINTING:
    case printing_state_t::ABSORBING_HEAT:
    case printing_state_t::PAUSED:
        EnableButton(BtnSocket::Left);
        break;
    case printing_state_t::ABORTING:
        DisableButton(BtnSocket::Left);
        break;
    default:
        DisableButton(BtnSocket::Left);
        break;
    }
}

void screen_printing_data_t::set_stop_icon_and_label() {
    switch (GetState()) {
    case printing_state_t::STOPPED:
    case printing_state_t::PRINTED:
        EnableButton(BtnSocket::Right);
        SetButtonIconAndLabel(BtnSocket::Right, BtnRes::Home, LabelRes::Home);
        break;
    case printing_state_t::PAUSING:
    case printing_state_t::RESUMING:
        DisableButton(BtnSocket::Right);
        SetButtonIconAndLabel(BtnSocket::Right, BtnRes::Stop, LabelRes::Stop);
        break;
    case printing_state_t::REHEATING:
        EnableButton(BtnSocket::Right);
        SetButtonIconAndLabel(BtnSocket::Right, BtnRes::Stop, LabelRes::Stop);
        break;
    case printing_state_t::ABORTING:
        DisableButton(BtnSocket::Right);
        break;
    default:
        EnableButton(BtnSocket::Right);
        SetButtonIconAndLabel(BtnSocket::Right, BtnRes::Stop, LabelRes::Stop);
        break;
    }
}

void screen_printing_data_t::change_print_state() {
    printing_state_t st = printing_state_t::COUNT;

    switch (marlin_vars()->print_state) {
    case State::Idle:
    case State::WaitGui:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
    case State::PrintPreviewConfirmed:
    case State::PrintPreviewQuestions:
    case State::PrintPreviewToolsMapping:
    case State::PrintInit:
        st = printing_state_t::INITIAL;
        break;
    case State::Printing:
        if (bed_preheat.is_waiting()) {
            st = printing_state_t::ABSORBING_HEAT;
        } else {
            st = printing_state_t::PRINTING;
        }
        break;
    case State::PowerPanic_AwaitingResume:
    case State::Paused:
        // stop_pressed = false;
        st = printing_state_t::PAUSED;
        break;
    case State::Pausing_Begin:
    case State::Pausing_Failed_Code:
    case State::Pausing_WaitIdle:
    case State::Pausing_ParkHead:
        st = printing_state_t::PAUSING;
// When print is paused, progress screen needs to reinit it's thumbnail file handler
// because USB removal error crashes file handler access. Progress screen should not be enabled during pause -> reinit on EVERY pause
#if defined(USE_ILI9488)
        print_progress.Pause();
#endif
        break;
    case State::Resuming_Reheating:
        stop_pressed = false;
        st = printing_state_t::REHEATING;
        break;
    case State::Resuming_Begin:
    case State::Resuming_UnparkHead_XY:
    case State::Resuming_UnparkHead_ZE:
    case State::CrashRecovery_Begin:
    case State::CrashRecovery_Retracting:
    case State::CrashRecovery_Lifting:
    case State::CrashRecovery_ToolchangePowerPanic:
    case State::CrashRecovery_XY_Measure:
    case State::CrashRecovery_Tool_Pickup:
    case State::CrashRecovery_XY_HOME:
    case State::CrashRecovery_HOMEFAIL:
    case State::CrashRecovery_Axis_NOK:
    case State::CrashRecovery_Repeated_Crash:
    case State::PowerPanic_Resume:
        stop_pressed = false;
        st = printing_state_t::RESUMING;
#ifdef USE_ILI9488
        print_progress.Resume();
#endif
        break;
    case State::Aborting_Begin:
    case State::Aborting_WaitIdle:
    case State::Aborting_ParkHead:
    case State::Aborting_Preview:
        stop_pressed = false;
        st = printing_state_t::ABORTING;
        break;
    case State::Finishing_WaitIdle:
    case State::Finishing_ParkHead:
        st = printing_state_t::PRINTING;
        break;
    case State::Aborted:
        stop_pressed = false;
        st = printing_state_t::STOPPED;
        break;
    case State::Finished:
    case State::Exit:
        st = printing_state_t::PRINTED;
        break;
    case State::PowerPanic_acFault:
    case State::SerialPrintInit:
        // this state is never reached
        __builtin_unreachable();
        return;
    }
    if (stop_pressed) {
        st = printing_state_t::ABORTING;
    }
    if (state__readonly__use_change_print_state != st) {
        state__readonly__use_change_print_state = st;
        set_pause_icon_and_label();
        set_tune_icon_and_label();
        set_stop_icon_and_label();
    }
    if (st == printing_state_t::PRINTED || st == printing_state_t::STOPPED || st == printing_state_t::PAUSED) {
        Odometer_s::instance().force_to_eeprom();
    }
}
