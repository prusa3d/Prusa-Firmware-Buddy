#pragma once
#include <cstdint>
#include <utility>
#include "gui_leds.hpp"
#include <optional>
#include <variant>

enum class AnimationStateExternal {
    InProgress,
    Ended
};

enum class AnimationStateInternal {
    Starting,
    InProgress,
    Ending,
    Ended,
};

class Animation {
public:
    Animation() = default;
    virtual void Step(const std::pair<uint16_t, uint16_t> &leds_to_run) = 0;

    virtual void EndAnimation();
    void KillAnimation();
    void StartAnimation();

    AnimationStateExternal GetState();

protected:
    AnimationStateInternal state = AnimationStateInternal::Starting;
    static void writeColorToLeds(const leds::ColorRGBW &toSet, const std::pair<uint16_t, uint16_t> &leds_to_run);
};

class Fading : public Animation {
public:
    ~Fading() = default;
    Fading(const Fading &other);
    Fading(leds::ColorRGBW color_, uint16_t period_);
    void Step(const std::pair<uint16_t, uint16_t> &leds_to_run) override;
    void EndAnimation() override;

private:
    leds::ColorRGBW calculateColor(uint32_t ticks);
    leds::ColorRGBW color;
    uint32_t startTime = 0;
    uint16_t period;
};

class SolidColor : public Animation {
public:
    uint32_t constexpr static fade_length = 500;
    ~SolidColor() = default;
    SolidColor(leds::ColorRGBW color)
        : color(color) {};
    SolidColor(const SolidColor &other)
        : color(other.color) {};
    void EndAnimation() override;
    void Step(const std::pair<uint16_t, uint16_t> &leds_to_run) override;

private:
    leds::ColorRGBW color;
    uint32_t end_started = 0;
};

using Animations = std::variant<Fading, SolidColor>;
