#pragma once
#include "marlin_client.hpp"
#include "animator.hpp"
#include <utility>
#include <type_traits>
#include <variant>
#include "led_types.h"
#include <mutex>
#include <freertos/mutex.hpp>
#include <gui/led_animations/animation_model.hpp>

namespace leds {
PrinterState mpsToAnimationState(marlin_server::State state);

AnimatorLCD::AnimationGuard start_animation(Animations animation, int priority);
AnimatorLCD::AnimationGuard start_animation(PrinterState state, int priority);
Animations get_animation(PrinterState state);
} // namespace leds

class PrinterStateAnimation {
public:
    PrinterStateAnimation() = default;
    static PrinterStateAnimation &Access();

    static void Update() {
        Access().update();
    }
    static AnimatorLCD::AnimationGuard force_printer_state(PrinterState state) {
        auto animation = leds::get_animation(state);
        return leds::start_animation(animation, 1);
    }
    void static force_printer_state_until(PrinterState state, PrinterState end_when) {
        std::lock_guard lock(Access().mutex);
        Access().change_animation_on = end_when;
        Access().changeAnimation(state);
    }
    static void set_animation([[maybe_unused]] PrinterState state, [[maybe_unused]] Animation_model animation) {
        Access().reload();
    }

private:
    freertos::Mutex mutex;
    PrinterState oldState = PrinterState::Printing;
    AnimatorLCD::AnimationGuard active_animation;
    std::optional<PrinterState> change_animation_on;

    /**
     * Checks if animations are identical
     * @return  true if they are the same false otherwise
     */
    bool check_animations(PrinterState state1, PrinterState state2);

    void reload() {
        std::lock_guard lock(mutex);
        PrinterState state = leds::mpsToAnimationState(marlin_vars().print_state);
        oldState = state;
        changeAnimation(state);
    }
    void update() {
        std::lock_guard lock(mutex);

        PrinterState state = leds::mpsToAnimationState(marlin_vars().print_state);
        if (state != oldState && !change_animation_on.has_value()) {
            oldState = state;
            changeAnimation(state);
        } else if (change_animation_on.has_value() && state == change_animation_on.value()) {
            change_animation_on = std::nullopt;
            oldState = state;
            changeAnimation(state);
        }
    }

    void changeAnimation(PrinterState event) {
        Animations animation = leds::get_animation(event);
        active_animation.Stop();
        active_animation = leds::start_animation(animation, 0);
    }
};
