#include "requests_base.hpp"

#include <logging/log.hpp>
#include <feature/openprinttag/request_manager.hpp>
#include <bsod/bsod.h>

LOG_COMPONENT_DEF(OpenPrintTag, logging::Severity::info);

namespace buddy::openprinttag {

// Do mind sizeof(Request) greatly.
// The class will get allocated on the stack many times at the same time, so every byte counts.
static_assert(sizeof(Request) == 8);
static_assert(sizeof(TagRequest) == 12);

Request::~Request() {
    manager().remove_request({}, *this);
}

void Request::issue() {
    manager().add_request({}, *this);
}

void Request::fail() {
    manager().remove_request({}, *this);
    set_finished(std::unexpected { Error::other });
}

void Request::set_finished(std::expected<std::monostate, Error> result) {
    debug_assert(!finished_);
    finished_ = true;
    error_ = result.error_or(Error::_cnt);
}

Request::SerializeResult TagRequest::serialize(ManagerNoLockBadge badge, RequestID request_id, anfc::modbus::Request &request) {
    const auto device_info = manager().get_tag_device_info_nolock(badge, tool_tag_);
    if (!device_info.has_value()) {
        log_warning(OpenPrintTag, "tag not found for request");
        set_finished(std::unexpected(Request::Error::other));
        return std::nullopt;
    }

    serialize(request_id, device_info->tag_id, request);
    return device_info->device;
}

} // namespace buddy::openprinttag
