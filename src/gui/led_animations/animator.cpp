#include "animator.hpp"

Animation *AnimatorBase::get_current() {
    return curr_animation;
}
Animation *AnimatorBase::get_next() {
    return next_animation;
}
void AnimatorBase::Step() {
    std::lock_guard lock(mutex);

    // check if we have next animation
    // if yes, then start ending current animation and wait for it to end or kill it
    // if current animation has ended set is as current animation
    if (next_animation) {
        if (curr_animation == nullptr) {
            curr_animation = next_animation;
            curr_animation->StartAnimation();
            next_animation = nullptr;
        } else {

            switch (curr_animation->GetState()) {
            case AnimationStateExternal::InProgress:
                curr_animation->EndAnimation();
                break;
            case AnimationStateExternal::Ended:
                curr_animation = next_animation;
                curr_animation->StartAnimation();
                next_animation = nullptr;
                break;
            default:
                // we are waiting for the animation to end
                break;
            }
        }
    }

    // step current animation
    if (curr_animation) {
        curr_animation->Step(leds);
        // if current animation had ended we want to remove it from animator
        if (curr_animation->GetState() == AnimationStateExternal::Ended) {
            curr_animation = nullptr;
        }
    }
}
void AnimatorBase::start_animation(Animation *animation) {
    // can't start new animation when animator is not running or the animation is already running
    if (run && curr_animation != animation) {
        next_animation = animation;
    }
}
void AnimatorBase::pause_animator() {
    std::lock_guard lock(mutex);
    run = false;
    next_animation = nullptr;
    if (curr_animation) {
        curr_animation->EndAnimation();
    }
    save_run_state(run);
}
AnimatorLCD &Animator_LCD_leds() {
    static AnimatorLCD animator({ 1, 3 });
    return animator;
}
bool AnimatorBase::animator_state() {
    std::lock_guard lock(mutex);
    return load_run_state();
}

void AnimatorBase::panic_off() {
    std::lock_guard lock(mutex);
    run = false;
    next_animation = nullptr;
    if (curr_animation) {
        curr_animation->KillAnimation();
    }
}
