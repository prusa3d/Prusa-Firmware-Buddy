#include "window_dlg_wait.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include <DialogHandler.hpp>
#include <client_response.hpp>
#include <common/marlin_vars.hpp>
#include <feature/print_status_message/print_status_message_mgr.hpp>
#include <feature/print_status_message/print_status_message_formatter_buddy.hpp>
#include <utils/string_builder.hpp>

static constexpr EnumArray<PhaseWait, const char *, PhaseWait::_cnt> phase_texts {
    { PhaseWait::print_status_message, nullptr },
};

window_dlg_wait_t::window_dlg_wait_t(Rect16 rect, const string_view_utf8 &text)
    : IDialogMarlin(rect)
    , frame(this, text) {
}

window_dlg_wait_t::window_dlg_wait_t(fsm::BaseData data)
    : window_dlg_wait_t(string_view_utf8 {}) {
    Change(data);
}

void window_dlg_wait_t::Change(fsm::BaseData data) {
    const auto phase = static_cast<PhaseWait>(data.GetPhase());

    frame.set_text(_(phase_texts[phase]));

    show_print_status_message_ = (phase == PhaseWait::print_status_message);

    // Reset/clear the message
    print_status_message_[0] = '\0';
}

void window_dlg_wait_t::wait_for_gcodes_to_finish() {
    // If the wait is short enough, don't show the wait dialog - it would just blink the screen
    for (int i = 0; i < 5; i++) {
        if (!marlin_vars().is_processing.get()) {
            return;
        }
        osDelay(10);
    }

    // Then show a wait dialog
    wait_until({}, [] {
        // This one is important - it allows popping up a warning dialog on top of this one
        DialogHandler::Access().Loop();
        return !marlin_vars().is_processing.get();
    });
}

void window_dlg_wait_t::wait_until(const string_view_utf8 &second_string, const stdext::inplace_function<bool()> &until_f) {
    wait_until(second_string, [&](window_dlg_wait_t &) { return until_f(); });
}

void window_dlg_wait_t::wait_until(const string_view_utf8 &initial_text, const stdext::inplace_function<bool(window_dlg_wait_t &)> &until_f) {
    window_dlg_wait_t dlg(initial_text);
    Screens::Access()->gui_loop_until_dialog_closed([&] {
        if (until_f(dlg)) {
            dlg.close_dialog();
        }
    });
}

void window_dlg_wait_t::windowEvent(window_t *sender, GUI_event_t event, void *const param) {
    switch (event) {

    case GUI_event_t::LOOP:
        if (show_print_status_message_) {
            const auto msg = print_status_message().current_message();

            decltype(print_status_message_) new_msg;
            StringBuilder builder { new_msg };
            PrintStatusMessageFormatterBuddy::format(builder, msg.message);

            if (strcmp(print_status_message_.data(), new_msg.data())) {
                print_status_message_ = new_msg;

                set_text(string_view_utf8::MakeRAM(print_status_message_.data()));
            }
            break;
        }

    default:
        break;
    }

    IDialogMarlin::windowEvent(sender, event, param);
}
