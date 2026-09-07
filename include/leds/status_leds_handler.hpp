#pragma once

#include <utils/led_color.hpp>
#include <led_animation_controller/frame_animation.hpp>

#include <freertos/mutex.hpp>

namespace leds {

enum class StateAnimation : uint8_t {
    Idle,
    Printing,
    Finished,
    Aborted,
    Warning,
    PowerPanic,
    PowerUp,
#if PRINTER_IS_PRUSA_iX()
    WaitingForPrinter,
    WaitingForUser,
    Unloading,
    WaitingForFilamentRemoval,
    FilamentRemoved,
    Inserting,
    Loading,
#endif
    Error,
    _last = Error,
};

enum class AnimationType : uint8_t {
#if PRINTER_IS_PRUSA_iX()
    RunningLeft,
    RunningRight,
    PulsingLeft,
    PulsingRight,
    Alternating,
#endif
    Solid,
    Pulsing,
    _last = Pulsing,
};

class StatusLedsHandler {
public:
    static StatusLedsHandler &instance();

    /**
     * @brief Sets an error state, overriding whatever printer state is fetched from Marlin.
     *
     * There's currently no way of returning to a normal state (it's used for BSOD/RSOD).
     */
    void set_error();

    /**
     * @brief Sets current animation, lasting until the next state change.
     */
    void set_animation(StateAnimation state);

    /**
     * @brief Sets a custom animation, lasting until the next state change.
     */
    void set_custom_animation(const ColorRGBW &color, AnimationType type, uint16_t period_ms);

    ColorRGBW get_color() const;

    StateAnimation current_animation();

    bool get_active();
    void set_active(bool val);

    void update();

    std::span<const ColorRGBW, 3> led_data();

private:
    freertos::Mutex mutex;
    bool active { config_store().run_leds.get() };
    StateAnimation old_state { StateAnimation::Idle };
    bool is_error_state { false };

    std::array<FrameAnimation<3>::Params, 2> custom_params_banks;
    uint8_t custom_params_bank_index { 0 };

    ColorRGBW color;
};

} // namespace leds
