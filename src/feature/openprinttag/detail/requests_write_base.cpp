/// @file
#include "requests_write_base.hpp"

#include <bsod/bsod.h>
#include <feature/openprinttag/request_manager.hpp>
#include <cstring>
#include <prusa3d/nfc/command/Request_1_0.h>
#include <prusa3d/nfc/event/Event_1_0.h>
#include <utils/byte_utils.hpp>

namespace buddy::openprinttag {

using Error = Request::Error;

static void serialize_write_field_float(RequestID request_id, TagID tag_id, ToolTagField tool_tag_field, float value, anfc::modbus::Request &request) {
    prusa3d_nfc_command_Request_Request_1_0 object;
    memset(&object, 0, sizeof(object));
    object.request_id.value = request_id.to_underlying();
    object.request.write_field.field.tag.value = tag_id;
    object.request.write_field.field.section.value = std::to_underlying(tool_tag_field.section);
    object.request.write_field.field.field.value = tool_tag_field.field;
    object.request.write_field.value.float_ = value;
    prusa3d_nfc_util_Value_1_0_select_float__(&object.request.write_field.value);
    prusa3d_nfc_request_RequestData_1_0_select_write_field_(&object.request);
    request.data = {};
    auto buffer = std::as_writable_bytes(std::span { request.data });
    size_t size = buffer.size();
    if (prusa3d_nfc_command_Request_Request_1_0_serialize_(&object, reinterpret_cast<uint8_t *>(buffer.data()), &size) == 0) {
        request.size = static_cast<uint16_t>(size);
    } else {
        bsod_unreachable();
    }
}

static std::expected<std::monostate, Error> deserialize_write_field_result(Bytes event_data) {
    prusa3d_nfc_event_Event_1_0 event;
    size_t size = event_data.size();
    if (prusa3d_nfc_event_Event_1_0_deserialize_(&event, reinterpret_cast<const uint8_t *>(event_data.data()), &size) != 0) {
        return std::unexpected(Error::other);
    }
    if (!prusa3d_nfc_event_EventData_1_0_is_request_done_(&event.data)) {
        return std::unexpected(Error::other);
    }
    const auto &result = event.data.request_done.result;
    if (!prusa3d_nfc_request_RequestResult_1_0_is_write_field_(&result)) {
        return std::unexpected(Error::other);
    }
    const auto &error = result.write_field;
    if (error._error != prusa3d_nfc_util_ReaderError_1_0_NO_ERROR) {
        return std::unexpected(static_cast<Error>(error._error));
    }
    return {};
}

void WriteFloatFieldRequest::serialize(RequestID request_id, TagID tag_id, anfc::modbus::Request &request) {
    serialize_write_field_float(request_id, tag_id, tag_field_, value_, request);
}

void WriteFloatFieldRequest::complete(Bytes event_data) {
    const auto result = deserialize_write_field_result(event_data);
    set_finished(result);
}

} // namespace buddy::openprinttag
