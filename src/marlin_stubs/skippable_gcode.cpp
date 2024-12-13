#include "skippable_gcode.hpp"

#include <cassert>

SkippableGCode::Guard::Guard() {
    skippable_gcode().skip_requested_ = false;
    [[maybe_unused]] const auto was_running = skippable_gcode().is_running_.exchange(true);
    assert(!was_running);
}
SkippableGCode::Guard::~Guard() {
    [[maybe_unused]] const auto was_running = skippable_gcode().is_running_.exchange(false);
    assert(was_running);
}

bool SkippableGCode::Guard::is_skip_requested() const {
    return skippable_gcode().skip_requested_.load();
}

bool SkippableGCode::is_running() const {
    return is_running_.load();
}

void SkippableGCode::request_skip() {
    skip_requested_ = true;
}

SkippableGCode &skippable_gcode() {
    static SkippableGCode instance;
    return instance;
}
