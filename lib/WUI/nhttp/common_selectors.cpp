#include "common_selectors.h"

using http::Method;
using http::Status;
using std::nullopt;
using std::optional;

namespace nhttp::handler::selectors {

optional<ConnectionState> ValidateRequest::accept(const RequestParser &request) const {
    if (request.error_code != Status::UnknownStatus) {
        /*
         * BadRequest may mean badly formed/unparsed request. Always close in
         * that case since there might be leftovers of the unrecognized request
         * lying around.
         */
        return StatusPage(request.error_code, request.error_code == Status::BadRequest ? StatusPage::CloseHandling::ErrorClose : request.status_page_handling(), request.accepts_json);
    }

    if (request.method == Method::UnknownMethod) {
        return StatusPage(Status::MethodNotAllowed, request, "Unrecognized method");
    }
    return nullopt;
}

const ValidateRequest validate_request;

optional<ConnectionState> UnknownRequest::accept(const RequestParser &request) const {
    return StatusPage(Status::NotFound, request);
}

const UnknownRequest unknown_request;

} // namespace nhttp::handler::selectors
