#include <transfers/monitor.hpp>

#include <catch2/catch.hpp>
#include <cstring>

using namespace transfers;

TEST_CASE("Transfers slot collision") {
    Monitor monitor;

    // Get a first slot
    auto slot = monitor.allocate(Monitor::Type::Connect, "/usb/path.gcode", 1024);
    REQUIRE(slot.has_value());

    // Won't get another one once the first one is alive
    REQUIRE_FALSE(monitor.allocate(Monitor::Type::Link, "/usb/another.gcode", 2048).has_value());

    // But once we finish the previous one, we can get a new slot.
    slot.reset();
    REQUIRE(monitor.allocate(Monitor::Type::Connect, "/usb/another.gcode", 4026).has_value());
}

TEST_CASE("Transfers status watch") {
    Monitor monitor;

    REQUIRE_FALSE(monitor.status().has_value());
    // Won't get even a stale status, because there was no previous transfer.
    REQUIRE_FALSE(monitor.status(true).has_value());

    auto slot = monitor.allocate(Monitor::Type::Connect, "/usb/path.gcode", 1024);

    auto status = monitor.status();
    REQUIRE(status.has_value());

    // 0 from the time mock
    REQUIRE(status->start == 0);
    REQUIRE(status->expected == 1024);
    REQUIRE(status->download_progress.get_valid_size() == 0);
    REQUIRE(strcmp(status->destination, "/usb/path.gcode") == 0);
    auto old_id = status->id;
    REQUIRE(status->id == monitor.id());
    REQUIRE(status->type == Monitor::Type::Connect);

    status.reset();

    slot->progress(10);
    slot->progress(20);

    status = monitor.status();

    REQUIRE(status.has_value());
    REQUIRE(status->id == old_id);
    REQUIRE(status->download_progress.get_valid_size() == 30);

    status.reset();
    slot.reset();

    // We won't get the status now, no transfer in progress
    REQUIRE_FALSE(monitor.status().has_value());
    // We can get a peek into the stale transfer, though.
    status = monitor.status(true);
    REQUIRE(status.has_value());
    // The info is still intact and the ID is also not changed.
    REQUIRE(status->id == old_id);
    REQUIRE(status->download_progress.get_valid_size() == 30);
    REQUIRE(strcmp(status->destination, "/usb/path.gcode") == 0);
}

// Check that IDs change when the transfer changes
TEST_CASE("Transfer IDs") {
    Monitor monitor;

    REQUIRE_FALSE(monitor.id().has_value());

    auto old_slot = monitor.allocate(Monitor::Type::Connect, "/usb/path.gcode", 1024);
    REQUIRE(old_slot.has_value());

    auto status = monitor.status();
    REQUIRE(status.has_value());
    auto old_id = status->id;
    REQUIRE(monitor.id() == old_id);

    status.reset();
    old_slot.reset();

    REQUIRE_FALSE(monitor.id().has_value());

    auto new_slot = monitor.allocate(Monitor::Type::Connect, "/usb/path.gcode", 1024);
    REQUIRE(new_slot.has_value());

    status = monitor.status();
    REQUIRE(status.has_value());
    auto new_id = status->id;
    REQUIRE(old_id != new_id);
    REQUIRE(monitor.id() == new_id);

    status.reset();
    new_slot.reset();
}

TEST_CASE("Transfer history") {
    Monitor monitor;

    REQUIRE_FALSE(monitor.outcome(42).has_value());

    auto o = [&](Monitor::Outcome o) -> TransferId {
        auto slot = monitor.allocate(Monitor::Type::Connect, "/usb/path.gcode", 1024);
        REQUIRE(slot.has_value());

        auto status = monitor.status();
        REQUIRE(status.has_value());

        slot->done(o);
        return status->id;
    };

    auto id1 = o(Monitor::Outcome::Finished);
    REQUIRE(monitor.outcome(id1) == Monitor::Outcome::Finished);

    auto id2 = o(Monitor::Outcome::ErrorNetwork);
    REQUIRE(monitor.outcome(id2) == Monitor::Outcome::ErrorNetwork);
    // The id1 is still reachable
    REQUIRE(monitor.outcome(id1) == Monitor::Outcome::Finished);

    auto id3 = o(Monitor::Outcome::Stopped);
    REQUIRE(monitor.outcome(id3) == Monitor::Outcome::Stopped);
    // The id1 is still reachable

    // Now, pretend to the the bad thing and not specify an outcome
    auto slot = monitor.allocate(Monitor::Type::Connect, "/usb/path.gcode", 1024);
    REQUIRE(slot.has_value());
    auto id4 = monitor.id();
    REQUIRE(id4.has_value());

    slot.reset();

    // Nobody called the done() method, the default should be ErrorOther.
    REQUIRE(monitor.outcome(*id4) == Monitor::Outcome::ErrorOther);

    // Already out of history (yes, we abuse the knowledge of the history size in this test).
    REQUIRE_FALSE(monitor.outcome(id1).has_value());
}

TEST_CASE("Transfer Estimates") {
    Monitor monitor;

    auto slot = monitor.allocate(Monitor::Type::Connect, "/usb/path.gcode", 1024);
    REQUIRE(slot.has_value());
    slot->progress(512);

    auto status = monitor.status();
    REQUIRE(status.has_value());
    REQUIRE(50 == (100.0 * status->progress_estimate()));
}
