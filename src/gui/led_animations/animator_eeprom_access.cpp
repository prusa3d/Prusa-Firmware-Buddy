#include "animator.hpp"
#include <configuration_store.hpp>

bool AnimatorBase::load_run_state() {
    return config_store().run_leds.get();
}
void AnimatorBase::save_run_state(bool state) {
    config_store().run_leds.set(state);
}
