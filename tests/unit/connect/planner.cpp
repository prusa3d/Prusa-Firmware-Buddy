#include "marlin_client_mock.h"
#include "time_mock.h"
#include "mock_printer.h"

#include <planner.hpp>
#include <transfers/monitor.hpp>
#include <transfers/changed_path.hpp>
#include <pwm_utils.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace connect_client;
using std::get;
using std::get_if;
using std::holds_alternative;
using std::move;
using transfers::ChangedPath;
using transfers::Monitor;

namespace {

SharedBuffer buffer;
struct Test {
    Printer::Params params;
    MockPrinter printer;
    Planner planner;

    Test(ActionResult init_result = ActionResult::Ok)
        : params(params_idle())
        , printer(this->params)
        , planner(printer) {
        // Intro exchange
        event_info();
        planner.transfer_recovery_finished(std::nullopt);
        planner.action_done(init_result);
    }

    void event_type(EventType event_type) {
        const auto action = planner.next_action(buffer, nullptr);
        const auto *event = get_if<Event>(&action);
        REQUIRE(event != nullptr);
        REQUIRE(event->type == event_type);
    }

    void event_info() {
        event_type(EventType::Info);
    }

    Duration consume_sleep() {
        printer.config();
        auto sleep = planner.next_action(buffer, nullptr);
        auto sleep_typed = get_if<Sleep>(&sleep);
        REQUIRE(sleep_typed != nullptr);
        Duration orig_sleep = sleep_typed->milliseconds;
        // Mark config as not modified
        printer.config();
        // Pretend to sleep a bit by moving the clock.
        sleep_typed->perform(printer, planner);

        return orig_sleep - sleep_typed->milliseconds;
    }

    void consume_telemetry() {
        Action action = planner.next_action(buffer, nullptr);
        REQUIRE(holds_alternative<SendTelemetry>(action));
        planner.action_done(ActionResult::Ok);
    }
};

} // namespace

TEST_CASE("connect::Success scenario", "[connect]") {
    Test test;

    test.consume_telemetry();

    test.consume_sleep();
    // Planner produces sleeps in small chunk, so we would have to consume
    // several of them. Simply just pretend it took longer.
    advance_time_s(120);

    // But now, after we have slept, we shall send some more telemetries
    test.consume_telemetry();
}

TEST_CASE("connect::Retries early", "[connect]") {
    Test test(ActionResult::Failed);

    // It'll provide a small sleep before trying again.
    Duration sleep1 = test.consume_sleep();

    // Now it'll retry the event.
    test.event_info();
    test.planner.action_done(ActionResult::Failed);

    // If it fails again, it schedules a longer sleep.
    Duration sleep2 = test.consume_sleep();
    REQUIRE(sleep1 < sleep2);

    // If we succeed after few retries, we move on to the telemetry
    test.event_info();
    test.planner.action_done(ActionResult::Ok);

    // Back to telemetries after the action
    test.consume_telemetry();
}

TEST_CASE("connect::Reinit after several failures", "[connect]") {
    Test test;

    // Eventually, it stops trying to send the telemetry and goes back to trying to send Info
    Action action;
    do {
        action = test.planner.next_action(buffer, nullptr);
        test.planner.action_done(ActionResult::Failed);
        test.consume_sleep();
    } while (holds_alternative<SendTelemetry>(action));

    REQUIRE(holds_alternative<Event>(action));
    REQUIRE(get<Event>(action).type == EventType::Info);

    // We'll succeed eventually
    test.event_info();
    test.planner.action_done(ActionResult::Ok);

    // Goes back to telemetry after sucessful reinit
    test.consume_telemetry();
}

TEST_CASE("connect::Unknown / broken command refused", "[connect]") {
    Test test;

    // Commands come as replies to telemetries
    test.consume_telemetry();

    SECTION("Unknown") {
        test.planner.command(Command {
            1,
            UnknownCommand {},
        });
    }

    SECTION("Broken") {
        test.planner.command(Command {
            1,
            BrokenCommand {},
        });
    }

    test.event_type(EventType::Rejected);
    test.planner.action_done(ActionResult::Ok);

    test.consume_telemetry();
}

TEST_CASE("connect::Send info request", "[connect]") {
    Test test;

    // Commands come as replies to telemetries
    test.consume_telemetry();

    test.planner.command(Command {
        1,
        SendInfo {},
    });

    test.event_info();
}

TEST_CASE("connect::Submit gcode", "[connect]") {
    Test test;

    // Commands come as replies to telemetries
    test.consume_telemetry();

    auto borrow = buffer.borrow().value();
    auto command = Command::gcode_command(1, "M100\n  M200 X10 Y20 \r\n\r\n \nM300", move(borrow));

    test.planner.command(command);

    // Not processed yet, it processes in the background.
    REQUIRE(test.printer.submitted_gcodes.size() == 0);
    // First, it accepts the command.
    test.event_type(EventType::Accepted);
    test.planner.action_done(ActionResult::Ok);

    // We always send a telemetry after an event.
    test.consume_telemetry();

    // (Still no background processing with the Accepted, that happens right away).
    REQUIRE(test.printer.submitted_gcodes.size() == 0);

    // The gcode is processed as part of sleep.
    REQUIRE(test.consume_sleep() == 500);

    // Processed as part of the short-circuited sleep.
    REQUIRE(test.printer.submitted_gcodes.size() == 4);

    REQUIRE(test.printer.submitted_gcodes[0] == "M100");
    REQUIRE(test.printer.submitted_gcodes[1] == "M200 X10 Y20");
    REQUIRE(test.printer.submitted_gcodes[2] == "M300");
    // The "Cork" added for waiting
    REQUIRE(strncmp(test.printer.submitted_gcodes[3].c_str(), "M9933 C", 7) == 0);

    uint16_t cookie;
    REQUIRE(sscanf(test.printer.submitted_gcodes[3].c_str() + 7, "%" SCNu16, &cookie) == 1 /* Number of items parsed */);

    buddy::cork::tracker.mark_done(cookie);

    REQUIRE(test.consume_sleep() == 0);
    test.event_type(EventType::Finished);
}

TEST_CASE("connect::Background command resubmit", "[connect]") {
    Test test;

    test.consume_telemetry();

    auto borrow = buffer.borrow().value();
    auto command = Command::gcode_command(1, "M100\n  M200 X10 Y20 \r\n\r\n \nM300", move(borrow));

    test.planner.command(command);

    // Not processed yet, it processes in the background.
    REQUIRE(test.printer.submitted_gcodes.size() == 0);
    // First, it accepts the command.
    test.event_type(EventType::Accepted);
    test.planner.action_done(ActionResult::Ok);

    // We always send a telemetry after an event.
    test.consume_telemetry();

    // We try to send the same command again (server thinks we didn't get it or something)
    test.planner.command(Command {
        1,
        ProcessingThisCommand {},
    });

    // The situation is about the same as before
    REQUIRE(test.printer.submitted_gcodes.size() == 0);
    // First, it accepts the command.
    test.event_type(EventType::Accepted);
    test.planner.action_done(ActionResult::Ok);

    // We try to send the same command again (server thinks we didn't get it or something)
    test.planner.command(Command {
        2,
        ProcessingOtherCommand {},
    });

    // The situation is about the same as before
    REQUIRE(test.printer.submitted_gcodes.size() == 0);
    // First, it accepts the command.
    test.event_type(EventType::Rejected);
    test.planner.action_done(ActionResult::Ok);

    // But the background command is still being processed
    REQUIRE(test.planner.background_command_id() == 1);
}

TEST_CASE("connect::Transport ended", "[connect]") {
    Test test;

    auto slot = Monitor::instance.allocate(Monitor::Type::Connect, "/usb/stuff.gcode", 1024);
    REQUIRE(slot.has_value());
    auto id = Monitor::instance.id();
    REQUIRE(id.has_value());

    test.consume_telemetry();

    // As long as the transfer is running, nothing much happens
    auto action1 = test.planner.next_action(buffer, nullptr);
    REQUIRE(holds_alternative<Sleep>(action1));

    // Finish the transfer
    slot->done(Monitor::Outcome::Finished);
    slot.reset();

    // Now that the transfer is done, we get an event about it.
    auto action2 = test.planner.next_action(buffer, nullptr);
    auto event = get_if<Event>(&action2);
    REQUIRE(event != nullptr);
    REQUIRE(event->type == EventType::TransferFinished);
    REQUIRE(event->transfer_id == id);

    // Check the id is never using 32nd bit - should be always 0
    REQUIRE(((*event->transfer_id).to_uint32_t() & ~0x7FFFFFFF) == 0);
}

TEST_CASE("connect::Transport ended and started", "[connect]") {
    Test test;

    auto slot = Monitor::instance.allocate(Monitor::Type::Connect, "/usb/stuff.gcode", false, 1024);
    REQUIRE(slot.has_value());
    auto id = Monitor::instance.id();
    REQUIRE(id.has_value());

    test.consume_telemetry();

    // As long as the transfer is running, nothing much happens
    auto action1 = test.planner.next_action(buffer, nullptr);
    REQUIRE(holds_alternative<Sleep>(action1));

    // Finish the transfer
    slot->done(Monitor::Outcome::Finished);
    slot.reset();

    // Start a new one.
    slot = Monitor::instance.allocate(Monitor::Type::Link, "/usb/stuff.gcode", false, 1024);

    // The info/notification that the previous one ended is still available.
    auto action2 = test.planner.next_action(buffer, nullptr);
    auto event = get_if<Event>(&action2);
    REQUIRE(event != nullptr);
    REQUIRE(event->type == EventType::TransferFinished);
    REQUIRE(event->transfer_id == id);

    // Check the id is never using 32nd bit - should be always 0
    REQUIRE(((*event->transfer_id).to_uint32_t() & ~0x7FFFFFFF) == 0);
}

// When lost in history, we lose the notification, but doesn't crash or do anything extra weird.
TEST_CASE("connect::Transport ended, lost in history", "[connect]") {
    Test test;

    auto slot = Monitor::instance.allocate(Monitor::Type::Connect, "/usb/stuff.gcode", false, 1024);
    REQUIRE(slot.has_value());

    test.consume_telemetry();

    // As long as the transfer is running, nothing much happens
    auto action1 = test.planner.next_action(buffer, nullptr);
    REQUIRE(holds_alternative<Sleep>(action1));

    // Finish the transfer
    slot->done(Monitor::Outcome::Finished);
    slot.reset();

    // Start many new ones, to push the old one from the history.
    for (size_t i = 0; i < 5; i++) {
        slot = Monitor::instance.allocate(Monitor::Type::Link, "/usb/stuff.gcode", false, 1024);
        slot.reset();
    }

    // The info/notification that the previous one ended is still available.
    auto action2 = test.planner.next_action(buffer, nullptr);
    REQUIRE(holds_alternative<Sleep>(action2));
}

TEST_CASE("connect::FileInfo after created file", "[connect]") {
    Test test;
    test.consume_telemetry();

    ChangedPath::instance.changed_path("/usb/some/file.gcode", ChangedPath::Type::File, ChangedPath::Incident::Created);

    auto action = test.planner.next_action(buffer, nullptr);
    auto event = get_if<Event>(&action);
    REQUIRE(event != nullptr);
    REQUIRE(event->type == EventType::FileInfo);
    REQUIRE(event->is_file);
    INFO(event->path->path());
    REQUIRE(strcmp(event->path->path(), "/usb/some/file.gcode") == 0);
    REQUIRE(event->incident == ChangedPath::Incident::Created);
}

TEST_CASE("connect::FileChanged after deleted file", "[connect]") {
    Test test;
    test.consume_telemetry();

    ChangedPath::instance.changed_path("/usb/some/file.gcode", ChangedPath::Type::File, ChangedPath::Incident::Deleted);

    auto action = test.planner.next_action(buffer, nullptr);
    auto event = get_if<Event>(&action);
    REQUIRE(event != nullptr);
    REQUIRE(event->type == EventType::FileChanged);
    REQUIRE(event->is_file);
    INFO(event->path->path());
    REQUIRE(strcmp(event->path->path(), "/usb/some/file.gcode") == 0);
    REQUIRE(event->incident == ChangedPath::Incident::Deleted);
}

TEST_CASE("connect::FileChanged after created folder", "[connect]") {
    Test test;
    test.consume_telemetry();

    ChangedPath::instance.changed_path("/usb/some/folder/", ChangedPath::Type::Folder, ChangedPath::Incident::Created);

    auto action = test.planner.next_action(buffer, nullptr);
    auto event = get_if<Event>(&action);
    REQUIRE(event != nullptr);
    REQUIRE(event->type == EventType::FileChanged);
    REQUIRE_FALSE(event->is_file);
    INFO(event->path->path());
    REQUIRE(strcmp(event->path->path(), "/usb/some/folder/") == 0);
    REQUIRE(event->incident == ChangedPath::Incident::Created);
}

TEST_CASE("connect::FileChanged after deleted folder", "[connect]") {
    Test test;
    test.consume_telemetry();

    ChangedPath::instance.changed_path("/usb/some/folder/", ChangedPath::Type::Folder, ChangedPath::Incident::Deleted);

    auto action = test.planner.next_action(buffer, nullptr);
    auto event = get_if<Event>(&action);
    REQUIRE(event != nullptr);
    REQUIRE(event->type == EventType::FileChanged);
    INFO(event->path->path());
    REQUIRE(strcmp(event->path->path(), "/usb/some/folder/") == 0);
    REQUIRE_FALSE(event->is_file);
    REQUIRE(event->incident == ChangedPath::Incident::Deleted);
}

TEST_CASE("connect::FileChanged after multiple fs changes", "[connect]") {
    Test test;
    test.consume_telemetry();

    ChangedPath::instance.changed_path("/usb/some/folder/", ChangedPath::Type::Folder, ChangedPath::Incident::Deleted);
    ChangedPath::instance.changed_path("/usb/some/file.gcode", ChangedPath::Type::File, ChangedPath::Incident::Created);
    ChangedPath::instance.changed_path("/usb/some/name/another_file,gcode", ChangedPath::Type::File, ChangedPath::Incident::Deleted);
    ChangedPath::instance.changed_path("/usb/some/yet_another_file.gcode", ChangedPath::Type::File, ChangedPath::Incident::Deleted);
    ChangedPath::instance.changed_path("/usb/some/other_folder/", ChangedPath::Type::Folder, ChangedPath::Incident::Created);

    auto action = test.planner.next_action(buffer, nullptr);
    auto event = get_if<Event>(&action);
    REQUIRE(event != nullptr);
    REQUIRE(event->type == EventType::FileChanged);
    INFO(event->path->path());
    REQUIRE(strcmp(event->path->path(), "/usb/some/") == 0);
    REQUIRE_FALSE(event->is_file);
    REQUIRE(event->incident == ChangedPath::Incident::Combined);
}
// TODO: Tests for unknown commands and such

TEST_CASE("connect::Command Set value - chamber.target_temp set/unset logic", "[connect]") {
    SECTION("unset") {
        Test test;
        marlin_client::last_gcode.clear();
        auto command = Command { CommandId(0), SetValue { PropertyName::ChamberTargetTemp, 0, uint32_t(connect_client::Printer::ChamberInfo::target_temp_unset) } };
        test.planner.command(command);
        REQUIRE(marlin_client::last_gcode == "M141 S0");
    }

    SECTION("35") {
        Test test;
        marlin_client::last_gcode.clear();
        auto command = Command { CommandId(0), SetValue { PropertyName::ChamberTargetTemp, 0, uint32_t(35) } };
        test.planner.command(command);
        REQUIRE(marlin_client::last_gcode == "M141 S35");
    }

    SECTION("55") {
        Test test;
        marlin_client::last_gcode.clear();
        auto command = Command { CommandId(0), SetValue { PropertyName::ChamberTargetTemp, 0, uint32_t(55) } };
        test.planner.command(command);
        REQUIRE(marlin_client::last_gcode == "M141 S55");
    }
}

TEST_CASE("connect::Test ID limited to 31bits only", "[connect]") {
    Test test;

    // Create ID at the end of 31bit range and increment
    auto slot1 = Monitor::instance.allocate(Monitor::Type::Connect, "/usb/stuff.gcode", false, 0x7FFFFFFF);
    REQUIRE(slot1.has_value());
    auto id1 = Monitor::instance.id();
    REQUIRE(id1.has_value());
    if (id1.has_value()) {
        REQUIRE(((*id1).to_uint32_t() & ~0x7FFFFFFF) == 0);
    }

    auto slot2 = Monitor::instance.allocate(Monitor::Type::Link, "/usb/stuff.gcode", false);
    auto id2 = Monitor::instance.id();
    REQUIRE(id2.has_value());
    if (id2.has_value()) {
        REQUIRE(((*id2).to_uint32_t() & ~0x7FFFFFFF) == 0);
    }

    // Create ID with 32bit set
    auto slot3 = Monitor::instance.allocate(Monitor::Type::Connect, "/usb/stuff.gcode", false, 0x80000001);
    auto id3 = Monitor::instance.id();
    REQUIRE(id3.has_value());
    if (id3.has_value()) {
        REQUIRE(((*id3).to_uint32_t() & ~0x7FFFFFFF) == 0);
    }
}

namespace buddy {
extern PWM255OrAuto cooling_fans_pwm;
} // namespace buddy

TEST_CASE("connect::Command Set value - xbuddy_extension fan1, 2 set/unset logic", "[connect]") {
    SECTION("unset") {
        Test test;
        auto command = Command { CommandId(0), SetValue { PropertyName::ChamberFanPwmTarget, 0, int8_t(-1) } };
        test.planner.command(command);
        REQUIRE(buddy::cooling_fans_pwm == pwm_auto);
    }
    SECTION("0%") {
        Test test;
        auto command = Command { CommandId(0), SetValue { PropertyName::ChamberFanPwmTarget, 0, int8_t(0) } };
        test.planner.command(command);
        REQUIRE(buddy::cooling_fans_pwm == PWM255 { 0 });
    }
    SECTION("100%") {
        Test test;
        auto command = Command { CommandId(0), SetValue { PropertyName::ChamberFanPwmTarget, 0, int8_t(100) } };
        test.planner.command(command);
        REQUIRE(buddy::cooling_fans_pwm == PWM255 { 255 });
    }
}

namespace buddy {
extern uint8_t ledpwm;
} // namespace buddy

namespace leds {
extern uint8_t side_max_brightness;
}

TEST_CASE("connect::Command Set value - xbuddy_extension LED intensity logic", "[connect]") {
    SECTION("0%") {
        Test test;
        auto command = Command { CommandId(0), SetValue { PropertyName::ChamberLedIntensity, 0, int8_t(0) } };
        test.planner.command(command);
        REQUIRE(leds::side_max_brightness == 0);
    }
    SECTION("100%") {
        Test test;
        auto command = Command { CommandId(0), SetValue { PropertyName::ChamberLedIntensity, 0, int8_t(100) } };
        test.planner.command(command);
        REQUIRE(leds::side_max_brightness == 255);
    }
}

namespace buddy {
extern bool usbpower;
} // namespace buddy

TEST_CASE("connect::Command Set value - xbuddy_extension usb addon power logic", "[connect]") {
    SECTION("true") {
        Test test;
        auto command = Command { CommandId(0), SetValue { PropertyName::AddonPower, 0, true } };
        test.planner.command(command);
        REQUIRE(buddy::usbpower);
    }
    SECTION("false") {
        Test test;
        auto command = Command { CommandId(0), SetValue { PropertyName::AddonPower, 0, false } };
        test.planner.command(command);
        REQUIRE_FALSE(buddy::usbpower);
    }
}

// Regression test: print_start_cmd must not leak across jobs.
// BFW-8603.
//
// Scenario:
//   1. StartPrint command sets print_start_cmd; JobInfo carries start_cmd_id
//      and the real job_id.
//   2. Job A appears (has_job=true, job_id=A) -> start_cmd_id stays.
//   3. Job B replaces A (has_job=true, job_id=B) -> start_cmd_id is dropped.
TEST_CASE("connect::start_cmd_id does not leak across jobs", "[connect]") {
    Test test;

    // Make StartPrint acceptable: the file "exists" and start_print succeeds
    // with job_id 100.
    test.printer.is_valid_file_or_transfer_result = true;
    test.printer.start_print_result = static_cast<uint16_t>(100);

    // Commands come as replies to telemetries.
    test.consume_telemetry();

    // Build a SharedPath pointing to a fake file. SharedPath has no
    // const char* constructor; we have to borrow the buffer and fill it by
    // hand, same as the JSON parser does in production.
    auto path_borrow = buffer.borrow().value();
    strlcpy(reinterpret_cast<char *>(path_borrow.data()),
        "/usb/foo.gcode", path_borrow.size());

    // 1. StartPrint issued by Connect. print_start_cmd gets set to 42.
    //    The JobInfo now carries the real job_id returned by start_print().
    test.planner.command(Command { CommandId(42),
        StartPrint { SharedPath(move(path_borrow)), std::nullopt } });
    {
        const auto action = test.planner.next_action(buffer, nullptr);
        const auto *event = get_if<Event>(&action);
        REQUIRE(event != nullptr);
        REQUIRE(event->type == EventType::JobInfo);
        // job_id is intentionally empty here — including it would make
        // the server immediately re-request the JobInfo.
        REQUIRE_FALSE(event->job_id.has_value());
        REQUIRE(event->start_cmd_id == CommandId(42));
        test.planner.action_done(ActionResult::Ok);
    }

    // The job is already visible right after start_print() succeeds
    // (job_id is updated internally in the MarkStarted phase).
    test.params.has_job = true;
    test.params.job_id = 100;

    test.consume_telemetry();

    // 2. SendJobInfo for the Connect-started job still carries start_cmd_id
    //    because print_start_cmd is for *this* job.
    //
    //    The consume_sleep() tick lets the planner run its clearing check
    //    (which is a no-op here since job_id matches print_start_job_id).
    test.consume_sleep();
    test.planner.command(Command { CommandId(43), SendJobInfo { 100 } });
    {
        const auto action = test.planner.next_action(buffer, nullptr);
        const auto *event = get_if<Event>(&action);
        REQUIRE(event != nullptr);
        REQUIRE(event->type == EventType::JobInfo);
        REQUIRE(event->start_cmd_id == CommandId(42));
        test.planner.action_done(ActionResult::Ok);
    }

    test.consume_telemetry();

    // 3. Job A ends and job B starts locally — the CMD id is removed as stale.
    test.params.job_id = 200;
    test.consume_sleep();
    test.planner.command(Command { CommandId(44), SendJobInfo { 200 } });
    {
        const auto action = test.planner.next_action(buffer, nullptr);
        const auto *event = get_if<Event>(&action);
        REQUIRE(event != nullptr);
        REQUIRE(event->type == EventType::JobInfo);
        REQUIRE_FALSE(event->start_cmd_id.has_value());
        test.planner.action_done(ActionResult::Ok);
    }
}

TEST_CASE("connect::Command Set value - nozzle out-of-range tool index rejected", "[connect]") {
    // idx=7 exceeds the physical tools available in the test printer, so it must be rejected.
    Test test;
    auto command = Command { CommandId(0), SetValue { PropertyName::NozzleDiameter, 7, float(0.4) } };
    test.planner.command(command);
    test.event_type(EventType::Rejected);
    test.planner.action_done(ActionResult::Ok);
}

// Regression test: a chamber value changed via SET_VALUE must be reported
// promptly, not only at the next periodic full telemetry.
TEST_CASE("connect::Chamber change forces full telemetry", "[connect]") {
    Test test;

    test.consume_telemetry();
    test.consume_sleep();
    // Well below TELEMETRY_INTERVAL_FULL (5 minutes), so a Full telemetry
    // here can only come from change detection, not the periodic timer.
    advance_time_s(120);

    test.params.chamber_info.led_intensity = 42;

    const Action action = test.planner.next_action(buffer, nullptr);
    const auto *telemetry = get_if<SendTelemetry>(&action);
    REQUIRE(telemetry != nullptr);
    REQUIRE(telemetry->mode == SendTelemetry::Mode::Full);
}

TEST_CASE("connect::Connect duplicates command", "[connect]") {
    Test test;

    auto command = Command { CommandId(42), SetValue { PropertyName::ChamberFanPwmTarget, 0, int8_t(-1) } };
    test.planner.command(command);
    test.event_type(EventType::Finished);
    test.planner.action_done(ActionResult::Ok);
    // Once more... we don't do it again.
    test.planner.command(command);
    test.event_type(EventType::Rejected);
    test.planner.action_done(ActionResult::Ok);
    // A different command (by it's ID) is accepted again.
    command.id = CommandId(43);
    test.planner.command(command);
    test.event_type(EventType::Finished);
}
