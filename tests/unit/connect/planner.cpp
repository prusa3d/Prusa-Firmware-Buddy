#include "time_mock.h"
#include "mock_printer.h"

#include <planner.hpp>
#include <transfers/monitor.hpp>
#include <transfers/changed_path.hpp>
#include <feature/chamber/chamber.hpp>

#include <catch2/catch.hpp>

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

TEST_CASE("Success scenario") {
    Test test;

    test.consume_telemetry();

    test.consume_sleep();
    // Planner produces sleeps in small chunk, so we would have to consume
    // several of them. Simply just pretend it took longer.
    advance_time_s(120);

    // But now, after we have slept, we shall send some more telemetries
    test.consume_telemetry();
}

TEST_CASE("Retries early") {
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

TEST_CASE("Reinit after several failures") {
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

TEST_CASE("Unknown / broken command refused") {
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

TEST_CASE("Send info request") {
    Test test;

    // Commands come as replies to telemetries
    test.consume_telemetry();

    test.planner.command(Command {
        1,
        SendInfo {},
    });

    test.event_info();
}

TEST_CASE("Submit gcode") {
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
    //
    // The sleep doesn't get around to sleeping in fact.
    REQUIRE(test.consume_sleep() == 0);

    // Processed as part of the short-circuited sleep.
    test.event_type(EventType::Finished);
    REQUIRE(test.printer.submitted_gcodes.size() == 3);

    REQUIRE(test.printer.submitted_gcodes[0] == "M100");
    REQUIRE(test.printer.submitted_gcodes[1] == "M200 X10 Y20");
    REQUIRE(test.printer.submitted_gcodes[2] == "M300");
}

TEST_CASE("Background command resubmit") {
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

TEST_CASE("Transport ended") {
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
}

TEST_CASE("Transport ended and started") {
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

    // Start a new one.
    slot = Monitor::instance.allocate(Monitor::Type::Link, "/usb/stuff.gcode", 1024);

    // The info/notification that the previous one ended is still available.
    auto action2 = test.planner.next_action(buffer, nullptr);
    auto event = get_if<Event>(&action2);
    REQUIRE(event != nullptr);
    REQUIRE(event->type == EventType::TransferFinished);
    REQUIRE(event->transfer_id == id);
}

// When lost in history, we lose the notification, but doesn't crash or do anything extra weird.
TEST_CASE("Transport ended, lost in history") {
    Test test;

    auto slot = Monitor::instance.allocate(Monitor::Type::Connect, "/usb/stuff.gcode", 1024);
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
        slot = Monitor::instance.allocate(Monitor::Type::Link, "/usb/stuff.gcode", 1024);
        slot.reset();
    }

    // The info/notification that the previous one ended is still available.
    auto action2 = test.planner.next_action(buffer, nullptr);
    REQUIRE(holds_alternative<Sleep>(action2));
}

TEST_CASE("FileInfo after created file") {
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

TEST_CASE("FileChanged after deleted file") {
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

TEST_CASE("FileChanged after created folder") {
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

TEST_CASE("FileChanged after deleted folder") {
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

TEST_CASE("FileChanged after multiple fs changes") {
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

TEST_CASE("Command Set value - chamber.target_temp set/unset logic") {
    SECTION("unset") {
        Test test;
        auto command = Command { CommandId(0), SetValue { PropertyName::ChamberTargetTemp, 0, uint32_t(0) } };
        test.planner.command(command);
        REQUIRE_FALSE(buddy::chamber().target_temperature().has_value());
    }

    SECTION("35") {
        Test test;
        auto command = Command { CommandId(0), SetValue { PropertyName::ChamberTargetTemp, 0, uint32_t(35) } };
        test.planner.command(command);
        REQUIRE(buddy::chamber().target_temperature().has_value());
        REQUIRE(buddy::chamber().target_temperature().value() == 35);
    }

    SECTION("55") {
        Test test;
        auto command = Command { CommandId(0), SetValue { PropertyName::ChamberTargetTemp, 0, uint32_t(55) } };
        test.planner.command(command);
        REQUIRE(buddy::chamber().target_temperature().has_value());
        REQUIRE(buddy::chamber().target_temperature().value() == 55);
    }
}

namespace buddy {
extern std::optional<uint8_t> fan12pwm;
} // namespace buddy

TEST_CASE("Command Set value - xbuddy_extension fan1, 2 set/unset logic") {
    SECTION("unset") {
        Test test;
        auto command = Command { CommandId(0), SetValue { PropertyName::ChamberFanPwmTarget, 0, int8_t(-1) } };
        test.planner.command(command);
        REQUIRE_FALSE(buddy::fan12pwm.has_value());
    }
    SECTION("0%") {
        Test test;
        auto command = Command { CommandId(0), SetValue { PropertyName::ChamberFanPwmTarget, 0, int8_t(0) } };
        test.planner.command(command);
        REQUIRE(buddy::fan12pwm.value() == 0);
    }
    SECTION("100%") {
        Test test;
        auto command = Command { CommandId(0), SetValue { PropertyName::ChamberFanPwmTarget, 0, int8_t(100) } };
        test.planner.command(command);
        REQUIRE(buddy::fan12pwm.value() == 255);
    }
}

namespace buddy {
extern uint8_t ledpwm;
} // namespace buddy

namespace leds {
extern uint8_t side_max_brightness;
}

TEST_CASE("Command Set value - xbuddy_extension LED intensity logic") {
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

TEST_CASE("Command Set value - xbuddy_extension usb addon power logic") {
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
