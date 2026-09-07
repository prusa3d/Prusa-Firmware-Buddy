#include <catch2/catch_test_macros.hpp>

#include <test_utils/formatters.hpp>
#include <feature/openprinttag/filament_usage_tracker/filament_usage_tracker.hpp>
#include <feature/openprinttag/detail/requests_base.hpp>
#include <feature/openprinttag/requests_read.hpp>
#include <feature/filament_tracker/filament_tracker.hpp>
#include <freertos/timing.hpp>
#include <set>
#include <utils/byte_utils.hpp>

using namespace buddy;
using namespace buddy::openprinttag;

using Items = std::vector<uint8_t>;

using VirtualExtension = VirtualToolIndexExtension;
using PhysicalExtension = PhysicalToolIndexExtension;
using GcodeExtension = GcodeToolIndexExtension;

std::set<uint8_t> enabled_physical_tools;
std::set<uint8_t> enabled_virtual_tools;

template <>
bool PhysicalToolIndex::is_enabled() const {
    const auto &self = static_cast<const PhysicalToolIndex &>(*this);
    return enabled_physical_tools.contains(self.to_raw());
}

template <>
bool VirtualToolIndex::is_enabled() const {
    const auto &self = static_cast<const VirtualToolIndex &>(*this);
    return enabled_virtual_tools.contains(self.to_raw());
}

namespace std {
inline ostream &operator<<(ostream &os, VirtualToolIndex o) {
    os << "tool(" << (int)o.to_raw() << ")";
    return os;
}

} // namespace std

inline VirtualToolIndex tool(uint8_t i) {
    return VirtualToolIndex::from_raw(i);
}

inline ToolTag tag(uint8_t i) {
    // +1 to avoid 0b0 which is no_tag_hash
    return ToolTag { tool(i), ToolTag::UIDHash(i + 1) };
}

std::unordered_map<uint8_t, ToolTag> assigned_tags;

namespace buddy::openprinttag {

class FilamentUsageTrackerTester {

public:
    VirtualToolIndex current_tool() {
        return instance.current_tool_;
    }

private:
    FilamentUsageTracker &instance = filament_usage_tracker();
};

FilamentUsageTrackerTester tester;

} // namespace buddy::openprinttag

Request::~Request() {}

void Request::set_finished(std::expected<std::monostate, Error> result) {
    assert(!finished_);
    finished_ = true;
    error_ = result.error_or(Error::_cnt);
}

void Request::issue() {
}

void ReadFloatFieldRequest::serialize(RequestID, TagID, anfc::modbus::Request &) {}
void WriteFloatFieldRequest::serialize(RequestID, TagID, anfc::modbus::Request &) {}

void ReadFloatFieldRequest::complete(Bytes) {}
void WriteFloatFieldRequest::complete(Bytes) {}

Request::SerializeResult TagRequest::serialize(ManagerNoLockBadge badge, RequestID request_id, anfc::modbus::Request &request) {
    return std::nullopt;
}

std::optional<ToolTag> ToolTag::for_tool_assigned(VirtualToolIndex tool) {
    auto it = assigned_tags.find(tool.to_raw());
    if (it != assigned_tags.end()) {
        return it->second;
    } else {
        return std::nullopt;
    }
}

std::optional<ToolTag> ToolTag::for_tool_ephemeral(VirtualToolIndex tool) {
    return for_tool_assigned(tool);
}

/// Does one step for all tools
void step_tracker() {
    const auto old_jobs = executed_job_count;

    auto prev_tool = tester.current_tool();
    size_t steps = 0;

    do {
        prev_tool = tester.current_tool();
        steps++;

        freertos::millis_val += FilamentUsageTracker::step_limiter_interval_ms;
        filament_usage_tracker().step();

    } while (tester.current_tool() == prev_tool || tester.current_tool() != VirtualToolIndex::from_raw(0));

    // step() should have serviced each tool by one step, except when there was an async job issued
    CHECK(steps == VirtualToolIndex::count + (executed_job_count - old_jobs));
}

TEST_CASE("buddy::openprinttag::filament_usage_tracker") {
    auto &tracker = filament_usage_tracker();

    // Test that this does nothing
    step_tracker();
    for (VirtualToolIndex tool : VirtualToolIndex::all()) {
        CHECK(!tracker.is_tracking(tool));
    }

    assigned_tags.emplace(0, tag(0));

    const float full_length = 1000;
    const float full_weight = 500;
    const float pre_consumed_weight = 10;
    stub_data[tag(0).field(MainField::nominal_full_length)] = full_length;
    stub_data[tag(0).field(MainField::actual_netto_full_weight)] = full_weight;
    stub_data[tag(0).field(AuxField::consumed_weight)] = pre_consumed_weight;

    assigned_tags.emplace(0, tag(1));
    stub_data[tag(1).field(MainField::nominal_full_length)] = full_length;

    // The tracker should now pick up the new tools and start tracking
    step_tracker();
    CHECK(tracker.is_tracking(tool(0)));
    CHECK(tracker.remaining_filament_g(tool(0)) == full_weight - pre_consumed_weight);

    // Cannot track - missing full_legth
    CHECK(!tracker.is_tracking(tool(1)));

    filament_usage_tracker().flush({ .tools = AllTools {} });
    step_tracker();

    // The tracker had no usage to write, so nothing should have been written
    CHECK(write_count == 0);

    // Extrude standard amount
    {
        const float extruded_dist = 50;
        filament_tracker().extruded_distances[tool(0)] = extruded_dist;
        filament_usage_tracker().flush({ .tools = AllTools {} });
        step_tracker();
        REQUIRE(stub_data.contains(tag(0).field(AuxField::consumed_weight)));
        CHECK(stub_data.size() == 4);

        const float consumed_weight = pre_consumed_weight + extruded_dist / full_length * full_weight;
        CHECK(std::any_cast<float>(stub_data[tag(0).field(AuxField::consumed_weight)]) == consumed_weight);
        CHECK(tracker.remaining_filament_g(tool(0)) == full_weight - consumed_weight);
    }

    // Extrude more than what is supposed to be on the spool
    {
        filament_tracker().extruded_distances[tool(0)] = full_length + 100;
        filament_usage_tracker().flush({ .tools = AllTools {} });
        step_tracker();

        const float consumed_weight = pre_consumed_weight + (full_length + 100) / full_length * full_weight;
        CHECK(std::any_cast<float>(stub_data[tag(0).field(AuxField::consumed_weight)]) == consumed_weight);
        CHECK(consumed_weight > full_weight);

        // Should crop to zero
        CHECK(tracker.remaining_filament_g(tool(0)) == 0);
    }
}
