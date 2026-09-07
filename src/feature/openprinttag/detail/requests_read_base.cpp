/// @file
#include "requests_read_base.hpp"

#include <bsod/bsod.h>
#include <feature/openprinttag/request_manager.hpp>
#include <cstring>
#include <prusa3d/nfc/command/Request_1_0.h>
#include <prusa3d/nfc/event/Event_1_0.h>
#include <utils/byte_utils.hpp>

namespace buddy::openprinttag {

ToolTagField ReadFieldRequestBase::tag_field() const {
    const auto maybe_section = region();
    if (maybe_section) {
        return ToolTagField {
            .tag = tool_tag(),
            .section = *maybe_section,
            .field = field_,
        };
    }
    // this never happens, see ReadFieldRequestBase constructor
    bsod_unreachable();
}

using Error = Request::Error;

using Func = bool(const prusa3d_nfc_util_Value_1_0 *const);

/// Deserialize read_field response, returning the Value on success or an Error.
/// The event parameter is used for storage - the returned pointer points into it.
static std::expected<const prusa3d_nfc_util_Value_1_0 *, Error> deserialize_read_field_value(
    Bytes event_data,
    prusa3d_nfc_event_Event_1_0 &event,
    Func func) {
    size_t size = event_data.size();
    if (prusa3d_nfc_event_Event_1_0_deserialize_(&event, reinterpret_cast<const uint8_t *>(event_data.data()), &size) != 0) {
        return std::unexpected(Error::other);
    }
    if (!prusa3d_nfc_event_EventData_1_0_is_request_done_(&event.data)) {
        return std::unexpected(Error::other);
    }
    const auto &result = event.data.request_done.result;
    if (!prusa3d_nfc_request_RequestResult_1_0_is_read_field_(&result)) {
        return std::unexpected(Error::other);
    }
    const auto &value_or_error = result.read_field;
    if (prusa3d_nfc_util_ValueOrError_1_0_is_error_(&value_or_error)) {
        return std::unexpected(static_cast<Error>(value_or_error._error._error));
    }
    if (!func(&value_or_error.value)) {
        return std::unexpected(Error::other);
    }
    return &value_or_error.value;
}

static void serialize_read_field(RequestID request_id, TagID tag_id, ToolTagField tool_tag_field, uint8_t value_type, anfc::modbus::Request &request) {
    prusa3d_nfc_command_Request_Request_1_0 object;
    memset(&object, 0, sizeof(object));
    object.request_id.value = request_id.to_underlying();
    object.request.read_field.field.tag.value = tag_id;
    object.request.read_field.field.section.value = std::to_underlying(tool_tag_field.section);
    object.request.read_field.field.field.value = tool_tag_field.field;
    object.request.read_field.value_type.value = value_type;
    prusa3d_nfc_request_RequestData_1_0_select_read_field_(&object.request);
    request.data = {};
    auto buffer = std::as_writable_bytes(std::span { request.data });
    size_t size = buffer.size();
    if (prusa3d_nfc_command_Request_Request_1_0_serialize_(&object, reinterpret_cast<uint8_t *>(buffer.data()), &size) == 0) {
        request.size = static_cast<uint16_t>(size);
    } else {
        bsod_unreachable();
    }
}

void ReadInt32FieldRequest::serialize(RequestID request_id, TagID tag_id, anfc::modbus::Request &request) {
    serialize_read_field(
        request_id,
        tag_id,
        tag_field(),
        prusa3d_nfc_util_ValueType_1_0_INT32_,
        request);
}

void ReadInt32FieldRequest::complete(Bytes event_data) {
    prusa3d_nfc_event_Event_1_0 event;
    const auto value_or_error = deserialize_read_field_value(event_data, event, prusa3d_nfc_util_Value_1_0_is_int32__);
    if (!value_or_error) {
        return set_finished(std::unexpected(value_or_error.error()));
    }
    result_ = value_or_error.value()->int32_;
    set_finished({});
}

void ReadFloatFieldRequest::serialize(RequestID request_id, TagID tag_id, anfc::modbus::Request &request) {
    serialize_read_field(
        request_id,
        tag_id,
        tag_field(),
        prusa3d_nfc_util_ValueType_1_0_FLOAT_,
        request);
}

void ReadFloatFieldRequest::complete(Bytes event_data) {
    prusa3d_nfc_event_Event_1_0 event;
    const auto value_or_error = deserialize_read_field_value(event_data, event, prusa3d_nfc_util_Value_1_0_is_float__);
    if (!value_or_error) {
        return set_finished(std::unexpected(value_or_error.error()));
    }
    result_ = value_or_error.value()->float_;
    if (!std::isfinite(result_)) {
        // Only accept "real" float numbers
        return set_finished(std::unexpected(Error::wrong_field_type));
    }
    set_finished({});
}

void ReadEnumFieldRequestBase::serialize(RequestID request_id, TagID tag_id, anfc::modbus::Request &request) {
    serialize_read_field(
        request_id,
        tag_id,
        tag_field(),
        prusa3d_nfc_util_ValueType_1_0_INT32_,
        request);
}

void ReadEnumFieldRequestBase::complete(Bytes event_data) {
    prusa3d_nfc_event_Event_1_0 event;
    const auto value_or_error = deserialize_read_field_value(event_data, event, prusa3d_nfc_util_Value_1_0_is_int32__);
    if (!value_or_error) {
        return set_finished(std::unexpected(value_or_error.error()));
    }
    set_result(value_or_error.value()->int32_);
}

void ReadEnumArrayRequestBase::serialize(RequestID request_id, TagID tag_id, anfc::modbus::Request &request) {
    serialize_read_field(
        request_id,
        tag_id,
        tag_field(),
        prusa3d_nfc_util_ValueType_1_0_UINT16_ARRAY,
        request);
}

void ReadEnumArrayRequestBase::complete(Bytes event_data) {
    prusa3d_nfc_event_Event_1_0 event;
    const auto value_or_error = deserialize_read_field_value(event_data, event, prusa3d_nfc_util_Value_1_0_is_uint16_array_);
    if (!value_or_error) {
        return set_finished(std::unexpected(value_or_error.error()));
    }
    const auto &arr = value_or_error.value()->uint16_array;
    set_result(arr.elements, arr.count);
}

void ReadStringRequestBase::serialize(RequestID request_id, TagID tag_id, anfc::modbus::Request &request) {
    serialize_read_field(
        request_id,
        tag_id,
        tag_field(),
        prusa3d_nfc_util_ValueType_1_0_STRING,
        request);
}

void ReadStringRequestBase::complete(Bytes event_data) {
    prusa3d_nfc_event_Event_1_0 event;
    const auto value_or_error = deserialize_read_field_value(event_data, event, prusa3d_nfc_util_Value_1_0_is_string_);
    if (!value_or_error) {
        return set_finished(std::unexpected(value_or_error.error()));
    }
    const auto &str = value_or_error.value()->_string;
    auto len = std::min(static_cast<size_t>(str.count), buffer_.size());
    std::memcpy(buffer_.data(), str.elements, len);
    result_ = { buffer_.data(), len };
    set_finished({});
}

} // namespace buddy::openprinttag
