/// @file
#pragma once
#include "../printers.h"
#include <leds/side_strip.hpp>
#include <math.h>
#include <timing.h>
#include <optional>
#include <freertos/mutex.hpp>

#if PRINTER_IS_PRUSA_iX()
    /// PresentColor duration_ms value 0 means forever
    #define SIDE_STRIP_ENDLESS_CUSTOM_COLOR() 1
#else
    /// PresentColor duration_ms value 0 has no special meaning
    #define SIDE_STRIP_ENDLESS_CUSTOM_COLOR() 0
#endif

namespace leds {

/// This class controls the colors of the side XL's LEDs
/// Needless to say, this class shouldn't be here. It is here for two reasons
/// 1) We should reuse led_animations used by the xLCD's (and merge controlling those
///     two led strips). However, led_animations isn't ready and needs a lot of arch. changes
/// 2) We need to control side strip asap :)
class SideStripControl {
public:
    SideStripControl();

    void ActivityPing();
    void PresentColor(ColorRGBW color, uint32_t duration_ms, uint32_t transition_ms);

    void Tick();

    /// Set maximum brightness of the side leds white channel. The W brightness is limited to this value.
    /// 0 = disable side leds completely (even RGB status blinking), 255 = full
    void set_max_brightness(uint8_t set);
    uint8_t max_brightness();

    void set_dimming_enabled(bool set);

    /**
     * @brief Quickly turn off LEDs.
     * Useful for PowerPanic.
     */
    void PanicOff();

private:
    enum class State {
        Startup,
        Idle,
        Active,
        CustomColor,
#if SIDE_STRIP_ENDLESS_CUSTOM_COLOR()
        EndlessCustomColor,
#endif
        SetOff,
        Off,
    };

    struct HsvColor {
        uint8_t h;
        uint8_t s;
        uint8_t v;
    };

    static ColorRGBW HsvToRgb(HsvColor hsv);

    static HsvColor RgbToHsv(ColorRGBW rgb);

    class Transition {
        uint8_t from;
        uint8_t to;

    public:
        Transition(float from, float to)
            : from(from)
            , to(to) {
        }

        uint8_t Get(float progress) {
            float diff = to - from;
            return from + diff * EasyInEasyOutCubic(progress);
        }

        float EasyInEasyOutCubic(float x) {
            return x < 0.5f ? 4.f * x * x * x : 1.f - powf(-2.0f * x + 2.0f, 3.f) / 2.f;
        }
    };

    struct ColorTransition {
        Transition hue;
        Transition saturation;
        Transition value;
        Transition white;

        ColorTransition(HsvColor from, uint8_t fromWhite, HsvColor to, uint8_t toWhite)
            : hue(from.h, to.h)
            , saturation(from.s, to.s)
            , value(from.v, to.v)
            , white(fromWhite, toWhite) {
        }

        ColorTransition(ColorRGBW from, ColorRGBW to)
            : ColorTransition(RgbToHsv(from), from.w, RgbToHsv(to), to.w) {
        }

        ColorRGBW Get(float progress) {
            auto rgb = HsvToRgb({ hue.Get(progress), saturation.Get(progress), value.Get(progress) });
            rgb.w = white.Get(progress);
            return rgb;
        }
    };

    struct ActiveColorTransition {
        uint32_t start_timestamp;
        uint32_t duration_ms;
        ColorTransition transition;

        ActiveColorTransition(ColorRGBW from, ColorRGBW to, uint32_t duration_ms)
            : start_timestamp(ticks_ms())
            , duration_ms(duration_ms)
            , transition(from, to) {
            assert(duration_ms > 0);
        }

        uint32_t GetElapsedMs() {
            return ticks_ms() - start_timestamp;
        }

        float GetProgress() {
            return std::clamp(
                static_cast<float>(GetElapsedMs()) / static_cast<float>(duration_ms),
                0.0f, 1.0f);
        }

        bool IsFinished() {
            return GetProgress() >= 1.0f;
        }

        ColorRGBW GetCurrentColor() {
            return transition.Get(GetProgress());
        }
    };

    std::optional<ActiveColorTransition> current_transition;
    void TransitionToColor(ColorRGBW color, uint32_t transition_ms);
    State state = State::Startup;
    bool dimming_enabled = false;
    uint8_t max_brightness_ = 0;
    freertos::Mutex mutex;

    // Active State
    const int active_timeout_ms = 120 * 1000;
    std::optional<uint32_t> active_start_timestamp;

    // Custom Color State
    struct CustomColorState {
        ColorRGBW color;
        uint32_t transition_duration;
        uint32_t state_duration;
        uint32_t start_timestamp;
    };
    std::optional<CustomColorState> custom_color;
};

extern SideStripControl side_strip_control;

} // namespace leds
