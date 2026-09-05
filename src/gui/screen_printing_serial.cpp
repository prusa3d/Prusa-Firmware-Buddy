#include "screen_printing_serial.hpp"
#include "config.h"
#include "marlin_client.hpp"
#include <marlin_server_shared.h>
#include "filament.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "odometer.hpp"
#include "config_features.h"
#include "screen_menu_tune.hpp"
#include "print_time_module.hpp"
#include <serial_printing.hpp>
#include <option/has_crash_detection.h>
#include <guiconfig/GuiDefaults.hpp>
#include <feature/print_status_message/print_status_message_mgr.hpp>
#include <feature/print_status_message/print_status_message_formatter_buddy.hpp>
#include <utils/string_builder.hpp>

#if HAS_CRASH_DETECTION()
    #include "../Marlin/src/feature/prusa/crash_recovery.hpp"
#endif

// Layout constants for the serial printing screen
// Note: Buttons start at Y=185, so content must end before that
namespace {
constexpr int16_t MARGIN_X = 30;
constexpr int16_t COL_W = 200; // Width of each column

constexpr int16_t PROGRESS_BAR_Y = 50;
constexpr int16_t PROGRESS_BAR_H = 16;

// Row 1: Progress % (centered)
constexpr int16_t ROW1_VALUE_Y = 72;

// Row 2: Elapsed | Remaining
constexpr int16_t ROW2_LABEL_Y = 110;
constexpr int16_t ROW2_VALUE_Y = 126;

constexpr int16_t LABEL_H = 16;
constexpr int16_t VALUE_H = 22;

// Status message (needs H=20 for proper rendering)
constexpr int16_t MESSAGE_Y = 155;
constexpr int16_t MESSAGE_H = 20;
} // namespace

screen_printing_serial_data_t::screen_printing_serial_data_t()
    : ScreenPrintingModel(_(caption))
    , w_progress(this, Rect16(MARGIN_X, PROGRESS_BAR_Y, GuiDefaults::ScreenWidth - 2 * MARGIN_X, PROGRESS_BAR_H))
    // Row 1: Progress % (centered)
    , w_progress_value(this, Rect16(0, ROW1_VALUE_Y, GuiDefaults::ScreenWidth, VALUE_H), is_multiline::no)
    // Row 2: Elapsed (left) | Remaining (right)
    , w_elapsed_label(this, Rect16(MARGIN_X, ROW2_LABEL_Y, COL_W, LABEL_H), is_multiline::no)
    , w_elapsed_value(this, Rect16(MARGIN_X, ROW2_VALUE_Y, COL_W, VALUE_H), is_multiline::no)
    , w_remaining_label(this, Rect16(GuiDefaults::ScreenWidth - MARGIN_X - COL_W, ROW2_LABEL_Y, COL_W, LABEL_H), is_multiline::no)
    , w_remaining_value(this, Rect16(GuiDefaults::ScreenWidth - MARGIN_X - COL_W, ROW2_VALUE_Y, COL_W, VALUE_H), is_multiline::no)
    // Buffers
    , elapsed_buffer()
    , progress_buffer()
    , remaining_buffer()
    , w_message(this, Rect16(MARGIN_X, MESSAGE_Y, GuiDefaults::ScreenWidth - 2 * MARGIN_X, MESSAGE_H), is_multiline::no)
    , message_text()
    , current_message()
    , last_tick(0)
    , connection(connection_state_t::connected)
    , last_state(marlin_server::State::Aborted) {
    ClrMenuTimeoutClose();
    SetOnSerialClose();

    // Row 1: Progress %
    w_progress_value.SetAlignment(Align_t::Center());
    w_progress_value.set_font(Font::big);
    strlcpy(progress_buffer.data(), "0%", progress_buffer.size());
    w_progress_value.SetText(string_view_utf8::MakeRAM(progress_buffer.data()));

    // Row 2 left: Elapsed
    w_elapsed_label.SetText(_("Print Time"));
    w_elapsed_label.SetAlignment(Align_t::Center());
    w_elapsed_label.set_font(Font::small);
    w_elapsed_label.SetTextColor(COLOR_SILVER);
    w_elapsed_value.SetAlignment(Align_t::Center());
    w_elapsed_value.set_font(Font::big);
    strlcpy(elapsed_buffer.data(), "0:00:00", elapsed_buffer.size());
    w_elapsed_value.SetText(string_view_utf8::MakeRAM(elapsed_buffer.data()));

    // Row 2 right: Remaining
    w_remaining_label.SetText(_(PrintTime::EN_STR_COUNTDOWN));
    w_remaining_label.SetAlignment(Align_t::Center());
    w_remaining_label.set_font(Font::small);
    w_remaining_label.SetTextColor(COLOR_SILVER);
    w_remaining_value.SetAlignment(Align_t::Center());
    w_remaining_value.set_font(Font::big);
    strlcpy(remaining_buffer.data(), "--:--", remaining_buffer.size());
    w_remaining_value.SetText(string_view_utf8::MakeRAM(remaining_buffer.data()));

    // Setup status message
    w_message.SetAlignment(Align_t::Center());
    w_message.set_font(Font::small);
    w_message.SetTextColor(COLOR_ORANGE);
    message_text[0] = '\0';
    w_message.SetText(string_view_utf8::MakeRAM(message_text.data()));

    SetButtonIconAndLabel(BtnSocket::Right, BtnRes::Disconnect, LabelRes::Stop);
}

void screen_printing_serial_data_t::updateTimes() {
    uint32_t duration = marlin_vars().print_duration.get();

    // Update progress percentage
    uint8_t percent = marlin_vars().sd_percent_done.get();
    snprintf(progress_buffer.data(), progress_buffer.size(), "%d%%", percent);
    w_progress_value.SetText(string_view_utf8::MakeRAM(progress_buffer.data()));
    w_progress_value.Invalidate();

    // Update elapsed time
    PrintTime::print_formatted_duration(duration, elapsed_buffer, true);
    w_elapsed_value.SetText(string_view_utf8::MakeRAM(elapsed_buffer.data()));
    w_elapsed_value.Invalidate();

    // Update remaining time from M73
    uint32_t time_to_end = marlin_vars().time_to_end.get();
    if (time_to_end != marlin_server::TIME_TO_END_INVALID) {
        PrintTime::print_formatted_duration(time_to_end, remaining_buffer, true);
    } else {
        strlcpy(remaining_buffer.data(), "--:--", remaining_buffer.size());
    }
    w_remaining_value.SetText(string_view_utf8::MakeRAM(remaining_buffer.data()));
    w_remaining_value.Invalidate();
}

void screen_printing_serial_data_t::updateStatusMessage() {
    const auto new_msg = print_status_message().current_message();
    if (new_msg.message != current_message.message) {
        if (new_msg) {
            // Format the new message
            std::array<char, 64> new_text;
            StringBuilder sb(new_text);
            PrintStatusMessageFormatterBuddy::format(sb, new_msg.message);
            if (strcmp(message_text.data(), new_text.data()) != 0) {
                strlcpy(message_text.data(), new_text.data(), message_text.size());
                w_message.SetText(string_view_utf8::MakeRAM(message_text.data()));
                w_message.Invalidate();
            }
        } else {
            // Clear message
            message_text[0] = '\0';
            w_message.SetText(string_view_utf8::MakeRAM(message_text.data()));
            w_message.Invalidate();
        }
        current_message = new_msg;
    }
}

void screen_printing_serial_data_t::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    marlin_server::State state = marlin_vars().print_state;

    if (state != last_state) {
        switch (state) {
        case marlin_server::State::Paused:
            // print is paused -> show resume button
            SetButtonIconAndLabel(BtnSocket::Middle, BtnRes::Resume, LabelRes::Resume);
            EnableButton(BtnSocket::Middle);
            break;
        case marlin_server::State::Printing:
            // print is running -> show pause button
            SetButtonIconAndLabel(BtnSocket::Middle, BtnRes::Pause, LabelRes::Pause);
            EnableButton(BtnSocket::Middle);
            break;
        default:
            // Any other state means printer pausing or resuming, so just wait for this to finish
            DisableButton(BtnSocket::Middle);
            break;
        }

        last_state = state;
    }

    // Update displays on each loop iteration
    if (event == GUI_event_t::LOOP) {
        updateTimes();
        updateStatusMessage();
    }

    ScreenPrintingModel::windowEvent(sender, event, param);
}

void screen_printing_serial_data_t::tuneAction() {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuTune>);
}

void screen_printing_serial_data_t::pauseAction() {
    // pause or resume button, depenging on what state we are in
    marlin_server::State state = marlin_vars().print_state;
    switch (state) {
    case marlin_server::State::Paused:
        marlin_client::print_resume();
        break;
    case marlin_server::State::Printing:
        marlin_client::print_pause();
        break;
    default:
        break;
    }
}

void screen_printing_serial_data_t::stopAction() {
    if (MsgBoxWarning(_("Are you sure to stop this printing?"), Responses_YesNo, 1)
        != Response::Yes) {
        return;
    }

    // abort print, disable button and wait for screen to close from marlin server
    marlin_client::print_abort();
    DisableButton(BtnSocket::Right);
}
