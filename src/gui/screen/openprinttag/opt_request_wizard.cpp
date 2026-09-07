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

        bool aux_region_corrupt = false;

        auto tag_corrupt_error = [] {
            MsgBoxError(_("Data on the OpenPrintTag are corrupt. Operation failed."), Responses_Ok);
        };

        for (Request *req : multirequest.requests()) {
            if (!req->has_error()) {
                continue;
            }

            using Error = Request::Error;
            switch (req->error()) {

            case Error::data_too_big:
            case Error::wrong_field_type:
            case Error::field_not_present:
                // Standard errors that will be handled further down the line, can't do much about them
                break;

            case Error::other: {
                const auto r = MsgBoxError(_("Failed to read data from the tag."), { Response::Abort, Response::Retry });
                switch (r) {

                case Response::Abort:
                    return false;

                case Response::Retry:
                    goto continue_issue_loop;

                default:
                    bsod_unreachable();
                }
                break;
            }

            case Error::region_corrupt:
                if (req->region() == Region::auxiliary) {
                    aux_region_corrupt = true;
                    break;
                } else {
                    tag_corrupt_error();
                    return false;
                }

            case Error::write_protected:
            case Error::tag_invalid:
                tag_corrupt_error();
                return false;

            case Error::not_implemented:
            case Error::radio_disabled:
            case Error::_cnt:
                bsod_unreachable();
            }
        }

        if (aux_region_corrupt) {
            // TODO offer formatting AUX region
            tag_corrupt_error();
            return false;
        }

        return true;
    continue_issue_loop: // Just makes the main loop continue and retry
    }
}

} // namespace buddy::openprinttag
