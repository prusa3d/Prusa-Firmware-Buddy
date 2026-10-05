#include <feature/openprinttag/detail/requests_base.hpp>
#include <feature/openprinttag/detail/requests_read_base.hpp>
#include <feature/openprinttag/detail/requests_write_base.hpp>
#include <feature/openprinttag/tool_tag.hpp>

#include <cassert>

using namespace buddy::openprinttag;

Request::~Request() {}

void Request::set_finished(std::expected<std::monostate, Error> result) {
    assert(!finished_);
    finished_ = true;
    error_ = result.error_or(Error::_cnt);
}

void ReadEnumArrayRequestBase::complete(Bytes event_data) {}
void ReadEnumFieldRequestBase::complete(Bytes event_data) {}
void ReadFloatFieldRequest::complete(Bytes event_data) {}
void ReadInt64FieldRequest::complete(Bytes event_data) {}
void ReadStringRequestBase::complete(Bytes event_data) {}
void ReadColorFieldRequest::complete(Bytes event_data) {}
void ReadUUIDFieldRequest::complete(Bytes event_data) {}

Request::SerializeResult TagRequest::serialize(ManagerNoLockBadge badge, RequestID request_id, anfc::modbus::Request &request) {
    return std::nullopt;
}

void ReadEnumArrayRequestBase::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadEnumFieldRequestBase::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadFloatFieldRequest::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadInt64FieldRequest::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadStringRequestBase::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadColorFieldRequest::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadUUIDFieldRequest::serialize(RequestID, TagID, anfc::modbus::Request &) {}

void WriteFloatFieldRequest::serialize(RequestID, TagID, anfc::modbus::Request &) {}

void WriteFloatFieldRequest::complete(Bytes) {}
