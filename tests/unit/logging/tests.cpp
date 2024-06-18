#include "catch2/catch.hpp"
#include <logging/log.hpp>
#include "utils.hpp"

LOG_COMPONENT_DEF(comp_01, logging::Severity::debug);
LOG_COMPONENT_DEF(comp_02, logging::Severity::debug);

TEST_CASE("once defined, component can be discovered", "[logging]") {
    REQUIRE(&__log_component_comp_01 == logging::log_component_find("comp_01"));
}

TEST_CASE("searching for undefined component returns null", "[logging]") {
    REQUIRE(logging::log_component_find("undefined") == nullptr);
}

TEST_CASE("in-memory destination captures an event", "[logging]") {
    ScopedInMemoryLog in_memory_log;
    log_debug(comp_01, "test event %i", 1);
    REQUIRE(in_memory_log.logs.size() == 1);
}

TEST_CASE("log event has proper properties", "[logging]") {
    ScopedInMemoryLog in_memory_log;

    SECTION("basic properties") {
        log_debug(comp_02, "test event");
        auto captured_log = in_memory_log.logs.front();
        in_memory_log.logs.pop_front();
        REQUIRE(captured_log.message == "test event");
        REQUIRE(captured_log.severity == logging::Severity::debug);
    }

    SECTION("timestamp") {
        logging::Timestamp counter = { 20, 141 };
        auto scope_guard = with_log_platform_timestamp_get([&]() {
            auto copy = counter;
            if (++counter.us >= 1000000) {
                counter.us = 0;
                counter.sec++;
            }
            return copy;
        });

        log_debug(comp_02, "141");
        log_info(comp_02, "142");
        auto log_141 = in_memory_log.logs.front();
        in_memory_log.logs.pop_front();
        auto log_142 = in_memory_log.logs.front();
        in_memory_log.logs.clear();

        REQUIRE(log_141.timestamp.sec == 20);
        REQUIRE(log_141.timestamp.us == 141);
        REQUIRE(log_142.timestamp.sec == 20);
        REQUIRE(log_142.timestamp.us == 142);
    }

    SECTION("task id") {
        logging::TaskId task_id = 1;
        auto scope_guard = with_log_platform_task_id_get([&]() {
            return task_id++;
        });

        log_debug(comp_02, "task 1");
        log_info(comp_02, "task 2");
        auto log_1 = in_memory_log.logs.front();
        in_memory_log.logs.pop_front();
        auto log_2 = in_memory_log.logs.front();
        in_memory_log.logs.clear();

        REQUIRE(log_1.task_id == 1);
        REQUIRE(log_2.task_id == 2);
    }

    SECTION("message formatting") {
        log_debug(comp_02, "test event with float: %.2f and substring: %s", 3.14, "ss");
        auto captured_log = in_memory_log.logs.front();
        in_memory_log.logs.pop_front();
        REQUIRE(captured_log.message == "test event with float: 3.14 and substring: ss");
    }
}

LOG_COMPONENT_DEF(comp_filter, logging::Severity::warning);

TEST_CASE("log destination filtering", "[logging]") {
    ScopedInMemoryLog in_memory_log;

    for (int severity = (int)logging::Severity::debug; severity <= (int)logging::Severity::critical; severity++) {
        LOG_COMPONENT(comp_filter).lowest_severity = (logging::Severity)severity;

        log_debug(comp_filter, "debug");
        log_info(comp_filter, "info");
        log_warning(comp_filter, "warning");
        log_error(comp_filter, "error");
        log_critical(comp_filter, "critical");

        REQUIRE(in_memory_log.logs.size() == size_t((int)logging::Severity::critical + (int)logging::Severity::debug - severity));
        in_memory_log.logs.clear();
    }
}
