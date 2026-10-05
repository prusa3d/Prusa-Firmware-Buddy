/// @file
#pragma once

#include <feature/openprinttag/detail/defines.hpp>
#include <feature/openprinttag/detail/requests_read_base.hpp>
#include <feature/openprinttag/detail/requests_write_base.hpp>

namespace buddy::openprinttag {

template <FieldType>
struct FieldTypeTraits;

template <>
struct FieldTypeTraits<FieldType::string> {
    template <auto field>
    using ReadFieldRequest = ReadStringFieldRequest<FieldTraits<field>::max_length>;
};

template <>
struct FieldTypeTraits<FieldType::number> {
    template <auto field>
    using ReadFieldRequest = ReadFloatFieldRequest;

    template <auto field>
    using WriteFieldRequest = WriteFloatFieldRequest;
};

template <>
struct FieldTypeTraits<FieldType::int_> {
    template <auto field>
    using ReadFieldRequest = ReadInt64FieldRequest;
};

template <>
struct FieldTypeTraits<FieldType::enum_> {
    template <auto field>
    using ReadFieldRequest = ReadEnumFieldRequest<typename FieldTraits<field>::Enum>;
};

template <>
struct FieldTypeTraits<FieldType::enum_array> {
    template <auto field>
    using ReadFieldRequest = ReadEnumArrayFieldRequest<typename FieldTraits<field>::Enum, FieldTraits<field>::max_length>;
};

template <>
struct FieldTypeTraits<FieldType::color_rgba> {
    template <auto field>
    using ReadFieldRequest = ReadColorFieldRequest;
};

template <>
struct FieldTypeTraits<FieldType::uuid> {
    template <auto field>
    using ReadFieldRequest = ReadUUIDFieldRequest;
};
}; // namespace buddy::openprinttag
