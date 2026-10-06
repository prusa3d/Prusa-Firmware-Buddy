#include "opt_request_wizard.hpp"

#include <window_dlg_wait.hpp>
#include <window_msgbox.hpp>
#include <bsod/bsod.h>

namespace buddy::openprinttag {

bool multirequest_with_troubleshooting(buddy::openprinttag::MultiRequestBase &multirequest) {
    // TODO:
    // - Option to format the aux region if corrupt
    // - Workgroups support
    while (true) {
        multirequest.issue();

        window_dlg_wait_t::wait_until(_("Communicating with the OpenPrintTag..."), [&] {
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
