#pragma once

#include <catch2/catch_test_macros.hpp>

#include <feature/openprinttag/detail/requests_meta.hpp>
#include <feature/openprinttag/tool_tag.hpp>
#include <requests_data_stub.hpp>

namespace buddy::openprinttag {

template <CField auto field>
class ReadFieldRequest final : public FieldTypeTraits<FieldTraits<field>::field_type>::template ReadFieldRequest<field> {

public:
    using FieldTraits = openprinttag::FieldTraits<field>;
    using FieldTypeTraits = openprinttag::FieldTypeTraits<FieldTraits::field_type>;
    using BaseReadFieldRequest = FieldTypeTraits::template ReadFieldRequest<field>;

    template <typename... Args>
    explicit inline ReadFieldRequest(ToolTag tag, Args &&...args)
        : BaseReadFieldRequest(tag.field(field), std::forward<Args>(args)...) {
        const auto it = stub_data.find(tag.field(field));
        if (it == stub_data.end()) {
            this->set_finished(std::unexpected(Request::Error::field_not_present));

        } else if (const auto error = std::any_cast<Request::Error>(&it->second)) {
            this->set_finished(std::unexpected(*error));

        } else {
            const auto field_type = typeid(typename BaseReadFieldRequest::Value).name();
            const auto any_type = it->second.type().name();
            CAPTURE(field_type, any_type);
            REQUIRE(field_type == any_type);
            const auto value = std::any_cast<typename BaseReadFieldRequest::Value>(it->second);

            // Mirrors ReadStringRequestBase::complete
            if constexpr (FieldTraits::field_type == FieldType::string) {
                if (value.size() > FieldTraits::max_length) {
                    this->set_finished(std::unexpected(Request::Error::data_too_big));
                    return;
                }
            }

            this->result_ = value;
            this->set_finished(std::monostate {});
        }
    }
};

} // namespace buddy::openprinttag
