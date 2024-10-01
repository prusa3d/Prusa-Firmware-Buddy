#include "common_selectors.h"

using http::Method;
using http::Status;
using std::nullopt;
using std::optional;

namespace nhttp::handler::selectors {

Selector::Accepted ValidateRequest::accept(const RequestParser &request, Step &out) const {
    if (request.error_code != Status::UnknownStatus) {
        /*
         * BadRequest may mean badly formed/unparsed request. Always close in
         * that case since there might be leftovers of the unrecognized request
         * lying around.
         */
        out.next = StatusPage(request.error_code, request.error_code == Status::BadRequest ? StatusPage::CloseHandling::ErrorClose : request.status_page_handling(), request.accepts_json);
        return Accepted::Accepted;
    }

    if (request.method == Method::UnknownMethod) {
        out.next = StatusPage(Status::MethodNotAllowed, request, "Unrecognized method");
        return Accepted::Accepted;
    }
    return Accepted::NextSelector;
}

const ValidateRequest validate_request;

Selector::Accepted UnknownRequest::accept(const RequestParser &request, Step &out) const {
    out.next = StatusPage(Status::NotFound, request);
    return Accepted::Accepted;
}

const UnknownRequest unknown_request;

} // namespace nhttp::handler::selectors
