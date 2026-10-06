#include "requests_multi_base.hpp"

namespace buddy::openprinttag {

void MultiRequestBase::issue() {
    // Call issue() of individual requests
    for (auto *request : requests()) {
        request->issue();
    }
}

void MultiRequestBase::fail() {
    for (auto *request : requests()) {
        request->fail();
    }
}

MultiRequestBase::Report MultiRequestBase::report() const {
    debug_assert(finished());

    Report result;
    for (const Request *req : requests()) {
        if (!req->has_error()) {
            continue;
        }

        using Error = Request::Error;
        switch (req->error()) {

        case Error::data_too_big:
        case Error::wrong_field_type:
        case Error::field_not_present:
            result.any_field_specific_error = true;
            break;

        case Error::other:
        case Error::field_collision: {
            result.any_recoverable_error = true;
            result.any_significant_error = true;
            break;
        }

        case Error::region_corrupt:
            result.any_unrecoverable_error = true;
            result.any_significant_error = true;
            if (req->region() == Region::auxiliary) {
                result.aux_region_corrupt = true;
            }
            break;

        case Error::write_protected:
        case Error::tag_invalid:
            result.any_unrecoverable_error = true;
            result.any_significant_error = true;
            break;

        case Error::not_implemented:
        case Error::radio_disabled:
        case Error::_cnt:
            bsod_unreachable();
        }
    }

    return result;
}

} // namespace buddy::openprinttag
