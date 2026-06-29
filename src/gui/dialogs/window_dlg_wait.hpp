/// \file
#pragma once

#include "IDialogMarlin.hpp"
#include "string_view_utf8.hpp"
#include <inplace_function.hpp>
#include <client_response.hpp>

#include <gui/standard_frame/frame_wait.hpp>

class window_dlg_wait_t : public IDialogMarlin {
    FrameWait frame;

public:
    window_dlg_wait_t(Rect16 rect, const string_view_utf8 &text = {});

    window_dlg_wait_t(fsm::BaseData data);

    window_dlg_wait_t(const string_view_utf8 &text)
        : window_dlg_wait_t(GuiDefaults::DialogFrameRect, text) {}

    /// Shows the dialog and blocks the UI thread until all gcodes are finished
    /// Does this in a somewhat smart way that doesn't obstruct warnings
    static void wait_for_gcodes_to_finish();

    /// Displays a waiting dialog and blocks until @p until_f returns true
    static void wait_until(const string_view_utf8 &second_string, const stdext::inplace_function<bool()> &until_f);

    /// Like wait_until() but passes the dialog to the predicate so it can update
    /// the displayed text via set_text() between iterations.
    static void wait_until(const string_view_utf8 &initial_text, const stdext::inplace_function<bool(window_dlg_wait_t &)> &until_f);

    /// Update the displayed text and force a redraw.
    void set_text(const string_view_utf8 &text) {
        // window_text_t skips invalidation when handed the same buffer pointer
        // again, even if its contents changed — reset first to force a redraw.
        frame.set_text({});
        frame.set_text(text);
    }

    virtual void Change(fsm::BaseData) override;

protected:
    void windowEvent(window_t *sender, GUI_event_t event, void *const param) override;

private:
    /// Buffer for PrintStatusMessage shown in PhaseWait::print_status_message
    std::array<char, 256> print_status_message_;

    /// Whether the frame text should show PrintStatusMessage
    bool show_print_status_message_ = false;
};
