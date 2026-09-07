/// @file
#pragma once

#include <feature/openprinttag/detail/requests_meta.hpp>
#include <feature/openprinttag/tool_tag.hpp>

namespace buddy::openprinttag {

template <CField auto field>
class WriteFieldRequest final : public FieldTypeTraits<FieldTraits<field>::field_type>::template WriteFieldRequest<field> {

public:
    using FieldTraits = openprinttag::FieldTraits<field>;
    using FieldTypeTraits = openprinttag::FieldTypeTraits<FieldTraits::field_type>;
    using BaseWriteFieldRequest = FieldTypeTraits::template WriteFieldRequest<field>;

    template <typename... Args>
    explicit inline WriteFieldRequest(ToolTag tag, Args &&...args)
        : BaseWriteFieldRequest(tag.field(field), std::forward<Args>(args)...) {
    }
};

} // namespace buddy::openprinttag
