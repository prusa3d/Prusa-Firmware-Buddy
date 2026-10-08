#include "opt_request_wizard.hpp"

#include <window_dlg_wait.hpp>
#include <window_msgbox.hpp>
#include <bsod/bsod.h>
#include <timing.h>
#include <ScreenHandler.hpp>

namespace buddy::openprinttag {

bool multirequest_with_troubleshooting(buddy::openprinttag::MultiRequestBase &multirequest) {
    const auto wait_str = _("Communicating with the OpenPrintTag...");
    bool auto_retried = false;

    // TODO:
    // - Option to format the aux region if corrupt
    // - Workgroups support
    while (true) {
        if (Screens::Access()->is_close_pending()) {
            // The current screen is about to be closed, bail
            return false;
        }

        multirequest.issue();

        window_dlg_wait_t::wait_until(wait_str, [&] {
            return multirequest.finished();
        });

        if (!multirequest.finished()) {
            // Something closed our wait dlg -> interrupted -> fail
            return false;
        }
        const auto report = multirequest.report();
        if (report.any_unrecoverable_error) {
            // TODO offer formatting AUX region
            MsgBoxError(_("Data on the OpenPrintTag are corrupt. Operation failed."), Responses_Ok);
            return false;

        } else if (report.any_recoverable_error) {
            if (!auto_retried) {
                auto_retried = true;
                // Automatically retry if the user is not waiting too long
                // This is to reduce the need for the user to interact,
                // and also to give the system enough time to silently close the dialog if the tag was removed.

                // Wait a bit before reissuing everything, give the system time detect lost tag
                // Reissuing immediately would flood the reader, proioritzing command processing over discovery
                const auto wait_start_ms = ticks_ms();
                window_dlg_wait_t::wait_until(wait_str, [&] {
                    return ticks_diff(ticks_ms(), wait_start_ms) > 3000;
                });
                continue;
            }

            const auto r = MsgBoxError(_("Failed to read data from the tag."), { Response::Abort, Response::Retry });
            switch (r) {

            case Response::Abort:
            case Response::_none: // Can happen by closing the dialog through close_dialog() without response
                return false;

            case Response::Retry:
                continue;

            default:
                bsod_unreachable();
            }

        } else if (report.any_field_specific_error) {
            // Standard errors that will be handled further down the line, can't do much about them here
        }

        return true;
    }
}

} // namespace buddy::openprinttag
