#include "catch2/catch.hpp"
#include <logging/log.hpp>
#include "utils.hpp"

LOG_COMPONENT_DEF(comp, logging::Severity::debug);

// This test has defined LOG_LOWEST_SEVERITY as WARNING, so all INFO and DEBUG events should be compiled out
//
// If the log_debug line wouldn't be replaced by preprocessor for empty statement, the compilation of
// this unit would fail, as we would have a statement in the top level of a file.
//
// Therefore, if this file is successfully compiled and ran, this test silenty succeeded 8-)
log_debug(comp, "this would throw an compile time error");
log_info(comp, "this would throw an compile time error");

// similar test at runtime
TEST_CASE("log filtering", "[logging]") {
    ScopedInMemoryLog in_memory_log;

    log_debug(comp, "debug");
    log_info(comp, "info");
    log_warning(comp, "warning");
    log_error(comp, "error");
    log_critical(comp, "critical");

    REQUIRE(in_memory_log.logs.size() == 3);
    in_memory_log.logs.clear();
}
