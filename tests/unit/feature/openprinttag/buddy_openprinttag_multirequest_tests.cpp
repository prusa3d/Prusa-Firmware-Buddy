#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <feature/openprinttag/requests_read_multi.hpp>
#include <utils/byte_utils.hpp>

using namespace buddy::openprinttag;

thread_local std::vector<const Request *> request_log;

const ToolTag tool_tag { VirtualToolIndex::from_raw(0), 10 };

Request::~Request() {}

void Request::set_finished(std::expected<std::monostate, Error> result) {
    assert(!finished_);
    finished_ = true;
    error_ = result.error_or(Error::_cnt);
}

void Request::issue() {
    request_log.push_back(this);
}

void ReadInt32FieldRequest::complete(Bytes event_data) {}
void ReadFloatFieldRequest::complete(Bytes event_data) {}
void ReadStringRequestBase::complete(Bytes event_data) {}

void ReadInt32FieldRequest::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadFloatFieldRequest::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void ReadStringRequestBase::serialize(RequestID, TagID, anfc::modbus::Request &) {}

Request::SerializeResult TagRequest::serialize(ManagerNoLockBadge badge, RequestID request_id, anfc::modbus::Request &request) {
    return std::nullopt;
}

std::optional<ToolTag> ToolTag::for_tool_ephemeral(VirtualToolIndex tool) {
    return std::nullopt;
}

std::optional<ToolTag> ToolTag::for_tool_assigned(VirtualToolIndex tool) {
    return std::nullopt;
}

TEST_CASE("buddy::openprinttag::MultiRequest") {
    using Request = MultiReadFieldRequest<
        MainField::material_name,
        MainField::nominal_netto_full_weight,
        MainField::nominal_netto_full_weight,
        ValuePack<
            AuxField::consumed_weight,
            MainField::nominal_netto_full_weight> {}>;

    request_log.clear();
    Request r { tool_tag };

    // Requests should be issued only once issue() is called
    CHECK(request_log.empty());

    const auto test_issue = [&] {
        r.issue();

        // MultiReadRequest is supposed to deduplicate fields and issue them all in order
        REQUIRE(request_log.size() == 3);
        CHECK(request_log[0] == &r.request<MainField::material_name>());
        CHECK(request_log[1] == &r.request<MainField::nominal_netto_full_weight>());
        CHECK(request_log[2] == &r.request<AuxField::consumed_weight>());
    };

    {
        INFO("First issue");
        test_issue();
    }

    {
        // Test that repeating the issue() command does everything the same
        INFO("Second issue");
        request_log.clear();
        test_issue();
    }
}

TEST_CASE("buddy::openprinttag::MultiRequest grouping") {
    using Request = MultiReadFieldRequest<
        MainField::material_name,
        AuxField::consumed_weight,
        MainField::nominal_netto_full_weight>;

    request_log.clear();
    Request r { tool_tag };

    r.issue();

    // Multirequest should group main field requests together to reduce read cache misses
    REQUIRE(request_log.size() == 3);
    CHECK(request_log[0] == &r.request<MainField::material_name>());
    CHECK(request_log[1] == &r.request<MainField::nominal_netto_full_weight>());
    CHECK(request_log[2] == &r.request<AuxField::consumed_weight>());
}

TEST_CASE("buddy::openprinttag::MultiRequestRef") {
    using Request = MultiReadFieldRequest<
        MainField::material_name,
        MainField::nominal_netto_full_weight,
        MainField::nominal_netto_full_weight,
        AuxField::consumed_weight>;

    request_log.clear();
    Request r { tool_tag };

    using Ref = buddy::openprinttag::MultiReadFieldRequestRef<MainField::nominal_netto_full_weight, AuxField::consumed_weight>;
    Ref ref { r };
    CHECK(sizeof(ref) == sizeof(void *) * 2);
    CHECK(&ref.request<MainField::nominal_netto_full_weight>() == &r.request<MainField::nominal_netto_full_weight>());
    CHECK(&ref.request<AuxField::consumed_weight>() == &r.request<AuxField::consumed_weight>());
}
