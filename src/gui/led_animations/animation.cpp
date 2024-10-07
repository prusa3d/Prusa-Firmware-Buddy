#include "animation.hpp"

#include <utility>
#include "timing.h"

AnimationStateExternal Animation::GetState() {
    switch (state) {
    case AnimationStateInternal::InProgress:
    case AnimationStateInternal::Starting:
    case AnimationStateInternal::Ending:
        return AnimationStateExternal::InProgress;
    case AnimationStateInternal::Ended:
        return AnimationStateExternal::Ended;
    }
    return AnimationStateExternal::InProgress;
}
void Animation::StartAnimation() {
    state = AnimationStateInternal::Starting;
}
void Animation::EndAnimation() {
    if (state != AnimationStateInternal::Ended) {
        state = AnimationStateInternal::Ending;
    }
}
void Animation::KillAnimation() {
    state = AnimationStateInternal::Ended;
}
void Animation::writeColorToLeds(const leds::ColorRGBW &toSet, const std::pair<uint16_t, uint16_t> &leds_to_run) {
    for (size_t i = leds_to_run.first; i <= leds_to_run.second; i++) {
        leds::SetNth(toSet, static_cast<leds::index>(i));
    }
}

void Fading::Step(const std::pair<uint16_t, uint16_t> &leds_to_run) {
    uint32_t ticks = ticks_ms();
    switch (state) {
    case AnimationStateInternal::Starting:
        startTime = ticks_ms();
        state = AnimationStateInternal::InProgress;
        [[fallthrough]];
    case AnimationStateInternal::InProgress: {
        leds::ColorRGBW toSet = calculateColor(ticks);
        writeColorToLeds(toSet, leds_to_run);
        return;
    }
    case AnimationStateInternal::Ending: {

        uint32_t periodCount = (ticks - startTime) / (2 * period);
        // if we are still in last breathe we want to calculate the color
        leds::ColorRGBW toSet = periodCount < 1 ? calculateColor(ticks) : leds::ColorRGBW { 0 };

        if (periodCount >= 1) {
            state = AnimationStateInternal::Ended;
        }

        writeColorToLeds(toSet, leds_to_run);

        return;
    }
    case AnimationStateInternal::Ended:
        // turn off the leds
        writeColorToLeds({ 0 }, leds_to_run);
        return;
    }
}

leds::ColorRGBW Fading::calculateColor(uint32_t ticks) {
    uint32_t posInPeriod = (ticks - startTime) % period;
    uint32_t parity = ((ticks - startTime) / period) % 2;

    double e = parity == 0 ? (double)posInPeriod / (double)period : 1.0 - ((double)posInPeriod / period);
    return color * e;
}

void Fading::EndAnimation() {
    if (state == AnimationStateInternal::Ending || state == AnimationStateInternal::Ended) {
        return;
    }
    Animation::EndAnimation();
    uint32_t ticks = ticks_ms();
    uint32_t posInPeriod = (ticks - startTime) % period;
    uint32_t parity = ((ticks - startTime) / period) % 2;

    if (parity == 0) {
        // if ending, and we are in increasing part of fading, move to the same position, but in the decreasing part
        startTime = ticks - posInPeriod;
    } else {
        startTime = ticks - period - posInPeriod;
    }
}

Fading::Fading(leds::ColorRGBW color_, uint16_t period_)
    : color(color_)
    , period(period_) {
}

Fading::Fading(const Fading &other)
    : color(other.color)
    , period(other.period) {
}

void SolidColor::Step(const std::pair<uint16_t, uint16_t> &leds_to_run) {
    switch (state) {
    case AnimationStateInternal::Starting:
        state = AnimationStateInternal::InProgress;
        [[fallthrough]];
    case AnimationStateInternal::InProgress:
        writeColorToLeds(color, leds_to_run);
        break;
    case AnimationStateInternal::Ending: {

        uint32_t ticks = ticks_ms();
        uint32_t timeSinceStart = (ticks - end_started);
        if (timeSinceStart > fade_length) {
            state = AnimationStateInternal::Ended;
            writeColorToLeds({ 0 }, leds_to_run);
        }

        double e = 1.0 - ((double)timeSinceStart / fade_length);
        leds::ColorRGBW toSet = color * e;
        for (size_t i = leds_to_run.first; i <= leds_to_run.second; i++) {
            leds::SetNth(toSet, static_cast<leds::index>(i));
        }
        break;
    }
    case AnimationStateInternal::Ended:
        // turn off the leds
        writeColorToLeds({ 0 }, leds_to_run);
        break;
    }
}
void SolidColor::EndAnimation() {
    if (state == AnimationStateInternal::Ending || state == AnimationStateInternal::Ended) {
        return;
    }
    Animation::EndAnimation();
    end_started = ticks_ms();
}
