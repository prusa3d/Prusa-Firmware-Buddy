#pragma once
#include "animation.hpp"
#include <freertos/mutex.hpp>

#include <cstdint>
#include <utility>
#include "static_alocation_ptr.hpp"
#include <functional>
#include "bsod.h"
#include <mutex>
#include "bsod.h"
#include "config.h"

// allocates and deallocates memory space (one size) on statically allocated memory
template <class Space, size_t SIZE>
class SpaceAllocator {
    Space storage[SIZE];
    std::array<bool, SIZE> info;

public:
    SpaceAllocator() {
        info.fill(false);
    }
    Space *get_space() {
        for (size_t i = 0; i < SIZE; i++) {
            if (!info[i]) {
                info[i] = true;
                return &storage[i];
            }
        }
#if !PRINTER_IS_PRUSA_MK3_5() // TODO fix error codes
        fatal_error(ErrCode::ERR_SYSTEM_LED_ANIMATION_BAD_SPACE_MANAGEMENT);
#endif
        return nullptr;
    }
    void free_space(Space *space) {
        size_t i = space - storage;
        info[i] = false;
    }
};

class AnimatorBase {
public:
    AnimatorBase(std::pair<uint16_t, uint16_t> leds)
        : leds(std::move(leds)) {
        run = load_run_state();
    }

    void Step();

    Animation *get_current();
    Animation *get_next();

    void pause_animator(); //< Stops animation from running, they still can be added and removed, but they won't be run on the leds (blocks them from entering next_animation).
    bool animator_state();

    /**
     * @brief Quickly turn off LEDs.
     * Useful for PowerPanic.
     */
    void panic_off();

protected:
    template <typename... Ts>
    class MyContainer {
    private:
        alignas(Ts...) std::byte t_buff[std::max({ sizeof(Ts)... })];
    };

    using MemSpace = MyContainer<Animation, Fading>;
    using AnimationStorage = std::pair<std::pair<int, int>, Animation *>;

    void Stop() {
        curr_animation = nullptr;
        next_animation = nullptr;
    }
    void start_animation(Animation *animation);

    template <class T, class... Args>
    Animation *create_animation(MemSpace *space, Args &&...args); //< Creates animation

    bool load_run_state();
    void save_run_state(bool state);

    std::pair<uint16_t, uint16_t> leds;
    bool run;
    freertos::Mutex mutex;
    Animation *curr_animation = nullptr;
    Animation *next_animation = nullptr;
};

template <class T, class... Args>
Animation *AnimatorBase::create_animation(MemSpace *space, Args &&...args) {
    static_assert(sizeof(T) <= sizeof(MemSpace), "Error animation does not fit");
    return ::new (space) T(std::forward<Args>(args)...); //::new - global new (no other new defined elsewhere)
}

template <size_t COUNT>
class Animator : public AnimatorBase {
public:
    Animator(std::pair<uint16_t, uint16_t> leds)
        : AnimatorBase(std::move(leds)) {
        animations.fill({ { -1, -1 }, nullptr });
    }

    // Guards lifetime of animation, if it is destroyed the animation will be stopped
    struct AnimationGuard {
        AnimationGuard()
            : animation(nullptr)
            , animator(nullptr) {}

        Animation *animation;
        Animator<COUNT> *animator;
        AnimationGuard(Animation *animation, Animator *animator)
            : animation(animation)
            , animator(animator) {}

        AnimationGuard(const AnimationGuard &) = delete;

        AnimationGuard &operator=(const AnimationGuard &) = delete;

        AnimationGuard &operator=(AnimationGuard &&animationGuard) {
            animator = animationGuard.animator;
            animation = animationGuard.animation;
            animationGuard.animator = nullptr;
            animationGuard.animation = nullptr;
            return *this;
        }

        ~AnimationGuard() {
            Stop();
        }
        void Stop() {
            if (animation != nullptr && animator != nullptr) {
                animator->stop_animation(animation);
                animation = nullptr;
            }
        }
    };

    template <class T>
    AnimationGuard start_animations(const T &animation, int priority); //< Starts new animation
    void stop_animation(Animation *animation); //< Stops animation, it lets the animation end peacefully and then starts next animation in queue (if it has any) or leaves the leds turned off
    void start_animator();

private:
    void insert_animation(AnimationStorage animation); //< Inserts animation to animator
    Animation *get_first_animation(); //< Gets animation to run now
    void remove_animation(Animation *animation); //<removes animation from animator
    void update_next_animation(); //< checks if we have animation which should be running, but it is not and queues it up to be run
    int get_next_pos(int priority); //< Gets next position of animation with priority (when animation has equal priority, the newer animation is run (stack like behavior))
    void clear_space(); //< removes ended animations from animator

    SpaceAllocator<MemSpace, COUNT> space_allocator;
    std::array<AnimationStorage, COUNT> animations;
};

template <size_t COUNT>
template <class T>
typename Animator<COUNT>::AnimationGuard Animator<COUNT>::start_animations(const T &animation, int priority) {
    std::lock_guard lock(mutex);
    clear_space();

    auto *space = space_allocator.get_space();
    if (space) {
        int nexPos = get_next_pos(priority);
        Animation *savedAnimation = create_animation<T>(space, animation);
        insert_animation(std::pair { std::pair { priority, nexPos }, savedAnimation });
        update_next_animation();
        return { savedAnimation, this };
    }
    return { nullptr, this };
}

template <size_t COUNT>
void Animator<COUNT>::stop_animation(Animation *animation) {
    if (animation == nullptr) {
        return;
    }
    std::lock_guard lock(mutex);

    // stop current animation if the animation we want to stop is running
    if (curr_animation == animation) {
        animation->EndAnimation();
    }
    // if it queued to be next, remove it
    if (animation == next_animation) {
        next_animation = nullptr;
    }
    remove_animation(animation);
    update_next_animation();
    clear_space();
}

template <size_t COUNT>
void Animator<COUNT>::clear_space() {
    // loops over animations and checks if there is animation with -1 pos and -1 priority. If yes and the animation is not currently running it removes it. Works similarly to garbage colector
    for (auto &elem : animations) {
        if (elem.first == std::pair { -1, -1 } && elem.second != nullptr) {
            if (elem.second != curr_animation) {
                space_allocator.free_space(reinterpret_cast<MemSpace *>(elem.second));
                elem.second = nullptr;
            }
        }
    }
}
template <size_t COUNT>
void Animator<COUNT>::remove_animation(Animation *animation) {
    // sets the animation position and priority to -1. It indicates that when this animation ends its memory should be freed
    for (auto &iter : animations) {
        if (iter.second == animation) {
            iter.first = { -1, -1 };
        }
    }
}
template <size_t COUNT>
void Animator<COUNT>::update_next_animation() {
    Animation *animation = get_first_animation();
    start_animation(animation);
}

template <size_t COUNT>
Animation *Animator<COUNT>::get_first_animation() {
    // gets animation with the largest priority and position
    std::pair<int, int> max = { -1, -1 };
    int index = -1;
    for (size_t i = 0; i < animations.size(); i++) {
        if (animations[i].second != nullptr) {
            if (animations[i].first > max) {
                max = animations[i].first;
                index = i;
            }
        }
    }
    if (index == -1) {
        return nullptr;
    } else {
        return animations[index].second;
    }
}
template <size_t COUNT>
int Animator<COUNT>::get_next_pos([[maybe_unused]] int priority) {
    // finds all animation with the same priority and returns one larger than the max one.
    int pos = 0;
    for (auto &elem : animations) {
        if (elem.second != nullptr) {
            pos = std::max(std::get<0>(elem).second, pos);
        }
    }
    return pos + 1;
}
template <size_t COUNT>
void Animator<COUNT>::insert_animation(Animator<COUNT>::AnimationStorage animation) {
    // finds free space in animator and stores the animation inside it.
    for (auto &iter : animations) {
        if (iter.second == nullptr) {
            iter = animation;
            return;
        }
    }
}
template <size_t COUNT>
void Animator<COUNT>::start_animator() {
    std::lock_guard lock(mutex);
    run = true;
    update_next_animation();
    save_run_state(run);
}

using AnimatorLCD = Animator<5>;

AnimatorLCD &Animator_LCD_leds();
