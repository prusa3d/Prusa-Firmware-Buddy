/// @file
#pragma once

#include <array>
#include <string_view>

#include <feature/openprinttag/tool_tag.hpp>
#include <feature/openprinttag/detail/requests_base.hpp>
#include <openprinttag/opt_fields.hpp>
#include <bsod/bsod.h>
#include <utils/byte_utils.hpp>

namespace buddy::openprinttag {

class ReadFieldRequestBase : public TagRequest {

public:
    explicit ReadFieldRequestBase(ToolTagField tag_field)
        : TagRequest(tag_field.section, tag_field.tag)
        , field_(tag_field.field) {}

    ToolTagField tag_field() const;
    Field field() const { return field_; }

protected:
    const Field field_;
};

template <typename T, typename Parent = ReadFieldRequestBase>
class ReadRequestMixin : public Parent {

public:
    using Value = T;
    using Error = Request::Error;
    using Result = std::expected<Value, Error>;

public:
    /// Once @p finished, can be used to obtain the result
    /// Cannot be called before finished()
    Result result() const {
        debug_assert(this->finished());

        if (this->has_error()) {
            return std::unexpected(this->error());
        }

        return result_;
    }

protected:
    // Inherit constructors
    using Parent::Parent;

protected:
    Value result_;
};

class ReadInt32FieldRequest : public ReadRequestMixin<int32_t> {

public:
    // Inherit constructor
    using ReadRequestMixin<int32_t>::ReadRequestMixin;

    void serialize(RequestID, TagID, anfc::modbus::Request &) final;
    void complete(Bytes event_data) final;
};

class ReadEnumFieldRequestBase : public ReadFieldRequestBase {

public:
    // Inherit constructor
    using ReadFieldRequestBase::ReadFieldRequestBase;

    void serialize(RequestID, TagID, anfc::modbus::Request &request) final;
    void complete(Bytes) final;

protected:
    virtual void set_result(int32_t value) = 0;
};

template <typename Enum>
class ReadEnumFieldRequest : public ReadRequestMixin<Enum, ReadEnumFieldRequestBase> {

public:
    // Inherit constructor
    using ReadRequestMixin<Enum, ReadEnumFieldRequestBase>::ReadRequestMixin;

private:
    void set_result(int32_t value) final {
        // Validate the enum value is within capacity bounds
        if (value < 0 || static_cast<size_t>(value) >= ::openprinttag::enum_capacity<Enum>()) {
            this->set_finished(std::unexpected(Request::Error::field_not_present));
            return;
        }
        this->result_ = static_cast<Enum>(value);
        this->set_finished({});
    }
};

class ReadEnumArrayRequestBase : public ReadFieldRequestBase {

public:
    // Inherit constructor
    using ReadFieldRequestBase::ReadFieldRequestBase;

    void serialize(RequestID, TagID, anfc::modbus::Request &request) final;
    void complete(Bytes) final;

protected:
    virtual void set_result(const uint16_t *elements, size_t count) = 0;
};

template <typename Enum, size_t max_size>
class ReadEnumArrayFieldRequest : public ReadRequestMixin<std::span<const Enum>, ReadEnumArrayRequestBase> {

public:
    // Inherit constructor
    using ReadRequestMixin<std::span<const Enum>, ReadEnumArrayRequestBase>::ReadRequestMixin;

private:
    void set_result(const uint16_t *elements, size_t count) final {
        count = std::min(count, max_size);
        size_t valid_count = 0;
        const auto capacity = ::openprinttag::enum_capacity<Enum>();
        for (size_t i = 0; i < count; ++i) {
            // Validate each enum value is within capacity bounds
            if (elements[i] < capacity) {
                buffer_[valid_count++] = static_cast<Enum>(elements[i]);
            }
        }
        this->result_ = { buffer_.data(), valid_count };
        this->set_finished({});
    }

    std::array<Enum, max_size> buffer_;
};

class ReadFloatFieldRequest : public ReadRequestMixin<float> {

public:
    // Inherit constructor
    using ReadRequestMixin<float>::ReadRequestMixin;

    void serialize(RequestID, TagID, anfc::modbus::Request &) final;
    void complete(Bytes event_data) final;
};

class ReadStringRequestBase : public ReadRequestMixin<std::string_view> {

public:
    /// @param buffer buffer for storing the text data
    explicit ReadStringRequestBase(ToolTagField tag_field, std::span<char> buffer)
        : ReadRequestMixin(tag_field)
        , buffer_(buffer) {
    }

    void serialize(RequestID, TagID, anfc::modbus::Request &) final;
    void complete(Bytes event_data) final;

protected:
    inline void set_buffer(std::span<char> set) {
        buffer_ = set;
    }

private:
    /// Buffer for storing the text data.
    /// The actual result will then be a subspan of this buffer.
    std::span<char> buffer_;
};

template <size_t array_size>
class ReadStringFieldRequest : public ReadStringRequestBase {

public:
    explicit ReadStringFieldRequest(ToolTagField tag_field)
        : ReadStringRequestBase(tag_field, std::span<char> {}) {
        set_buffer(array_);
    }

private:
    std::array<char, array_size> array_;
};

} // namespace buddy::openprinttag
