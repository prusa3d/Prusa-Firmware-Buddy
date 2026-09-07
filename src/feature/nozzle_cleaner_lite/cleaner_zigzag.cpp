#include "cleaner_zigzag.hpp"

namespace nozzle_cleaner_lite {

ZigZag::ZigZag(const Pad &pad, float crossings_per_stroke, float start_phase)
    : pad_ { pad }
    , half_period_ { pad.length() / crossings_per_stroke }
    , along_ { pad.along_near }
    , wiped_ { std::fmod(std::abs(start_phase), 1.0f) * 2.0f * half_period_ } {}

ZigZag::Waypoint ZigZag::start() const {
    return { along_, across_at(wiped_) };
}

void ZigZag::begin_stroke(float along_to) {
    stroke_to_ = along_to;
    stroke_end_ = wiped_ + std::abs(along_to - along_);
    // The turn at wiped_ itself was already reached by the previous stroke
    next_turn_ = (std::floor(wiped_ / half_period_) + 1.0f) * half_period_;
}

std::optional<ZigZag::Waypoint> ZigZag::next() {
    if (!stroke_to_) {
        return std::nullopt;
    }

    // Waypoints are placed relative to the start of the stroke, so along_ and
    // wiped_ only advance once it completes.
    const float direction = *stroke_to_ > along_ ? 1.0f : -1.0f;

    if (next_turn_ < stroke_end_) {
        const Waypoint turn { along_ + direction * (next_turn_ - wiped_), across_at(next_turn_) };
        next_turn_ += half_period_;
        return turn;
    }

    const Waypoint stroke_end { *stroke_to_, across_at(stroke_end_) };
    along_ = *stroke_to_;
    wiped_ = stroke_end_;
    stroke_to_.reset();
    return stroke_end;
}

float ZigZag::across_at(float wiped) const {
    const float ramp = std::fmod(wiped, 2.0f * half_period_) / half_period_; // [0, 2)
    return pad_.across_min + pad_.width() * (ramp < 1.0f ? ramp : 2.0f - ramp);
}

} // namespace nozzle_cleaner_lite
