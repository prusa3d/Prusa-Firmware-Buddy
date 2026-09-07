/// @file

#include "requests_util.hpp"

#include <prusa3d/nfc/command/Request_1_0.h>
#include <bsod/bsod.h>
#include <utils/byte_utils.hpp>

namespace buddy::openprinttag {

Request::SerializeResult EnableRadioRequest::serialize(ManagerNoLockBadge, RequestID request_id, anfc::modbus::Request &request) {
    prusa3d_nfc_command_Request_Request_1_0 object;
    memset(&object, 0, sizeof(object));
    object.request_id.value = request_id.to_underlying();
    if (enable_) {
        prusa3d_nfc_request_RequestData_1_0_select_enable_radio_(&object.request);
    } else {
        prusa3d_nfc_request_RequestData_1_0_select_disable_radio_(&object.request);
    }
    request.data = {};
    auto buffer = std::as_writable_bytes(std::span { request.data });
    size_t size = buffer.size();
    if (prusa3d_nfc_command_Request_Request_1_0_serialize_(&object, reinterpret_cast<uint8_t *>(buffer.data()), &size) == 0) {
        request.size = static_cast<uint16_t>(size);
    } else {
        bsod_unreachable();
    }

    return device_;
}

void EnableRadioRequest::complete(Bytes) {
    // Radio enable/disable has no return data
    set_finished(std::monostate {});
}

} // namespace buddy::openprinttag
