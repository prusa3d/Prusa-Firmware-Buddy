#include "catch2/catch.hpp"
#include "animation.hpp"
#include "animator.hpp"
#include "timing_dummy.hpp"
#include "leds_dummy.hpp"
#include "color_matcher.hpp"

CATCH_REGISTER_ENUM(AnimationStateExternal, AnimationStateExternal::InProgress, AnimationStateExternal::Ended)

void fatal_error(const char *error, const char *module) {
    REQUIRE(false);
}

class DummyAnimation : public Animation {
public:
    void EndAnimation() override {
        Animation::EndAnimation();
        endIn = stepCounter + endIn;
    }
    DummyAnimation(int id)
        : id(id) {}
    DummyAnimation(const DummyAnimation &other)
        : endIn(-1)
        , stepCounter(0)
        , id(other.id) {}

    void Step(const std::pair<uint16_t, uint16_t> &leds_to_run) override {
        stepCounter++;
        if (state == AnimationStateInternal::Ending) {
            state = AnimationStateInternal::Ended;
        } else if (state == AnimationStateInternal::Starting) {
            state = AnimationStateInternal::InProgress;
        }
    }
    int endIn = 1;
    int stepCounter = 0;
    int id;
};

TEST_CASE("Animator memory management", "[Animator,Memory]") {
    Animator<2> animator({ 0, 3 });
    auto lifetime1 = animator.start_animations(DummyAnimation(1), 1);
    auto lifetime2 = animator.start_animations(DummyAnimation(2), 1);

    animator.Step();
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->id == 2);

    lifetime1.Stop();
    auto lifetime3 = animator.start_animations(DummyAnimation(3), 1);
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_next())->id == 3);
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->id == 2);

    lifetime3.Stop();
    auto lifetime4 = animator.start_animations(DummyAnimation(4), 1);
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_next())->id == 4);
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->id == 2);
    lifetime2.Stop();
    lifetime4.Stop();
}

TEST_CASE("Animator different priority", "[Animator]") {

    Animator<3> animator({ 0, 3 });
    REQUIRE(animator.get_current() == nullptr);
    REQUIRE(animator.get_next() == nullptr);

    DummyAnimation animation(1);

    auto lifetime = animator.start_animations<DummyAnimation>(animation, 1);
    REQUIRE(animator.get_current() == nullptr);
    REQUIRE(animator.get_next() != nullptr);
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_next())->id == 1);

    animator.Step();
    REQUIRE(animator.get_current() != nullptr);
    REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->stepCounter == 1);

    animator.Step();
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->stepCounter == 2);
    REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);

    DummyAnimation animation2(2);
    auto lifetime2 = animator.start_animations<DummyAnimation>(animation2, 2);
    REQUIRE(animator.get_next() != nullptr);
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_next())->id == 2);

    Animation *nextAnimation = animator.get_next();
    Animation *currAnimation = animator.get_current();
    REQUIRE(animator.get_current() != animator.get_next());
    REQUIRE(animator.get_next() != nullptr);

    animator.Step();
    REQUIRE(dynamic_cast<DummyAnimation *>(currAnimation)->stepCounter == 3);
    REQUIRE(animator.get_current() == nullptr);
    REQUIRE(animator.get_current() != animator.get_next());
    REQUIRE(currAnimation->GetState() == AnimationStateExternal::Ended);

    animator.Step();
    REQUIRE(animator.get_next() == nullptr);
    REQUIRE(animator.get_current() != animator.get_next());
    REQUIRE(animator.get_current() == nextAnimation);
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->stepCounter == 1);
    std::swap(nextAnimation, currAnimation); // animation positions has changed

    SECTION("stop in order") {

        lifetime2.Stop();
        REQUIRE(animator.get_next() != nullptr);
        REQUIRE(currAnimation->GetState() == AnimationStateExternal::InProgress);

        animator.Step();
        REQUIRE(animator.get_next() != nullptr);
        REQUIRE(animator.get_current() == nullptr);
        REQUIRE(currAnimation->GetState() == AnimationStateExternal::Ended);
        REQUIRE(nextAnimation->GetState() == AnimationStateExternal::Ended);

        animator.Step();
        currAnimation = nextAnimation;
        REQUIRE(animator.get_next() == nullptr);
        REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);
        REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->id == 1);

        lifetime.Stop();
        REQUIRE(animator.get_next() == nullptr);
        REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);

        animator.Step();
        REQUIRE(currAnimation->GetState() == AnimationStateExternal::Ended);
    }

    SECTION("stop out of order") {
        lifetime.Stop();
        REQUIRE(animator.get_next() == nullptr);
        REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);

        animator.Step();
        REQUIRE(animator.get_next() == nullptr);
        REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->id == 2);
        REQUIRE(nextAnimation->GetState() == AnimationStateExternal::Ended);

        animator.Step();
        REQUIRE(animator.get_next() == nullptr);
        REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);
        REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->id == 2);

        lifetime2.Stop();
        REQUIRE(animator.get_next() == nullptr);
        REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);
        currAnimation = animator.get_current();

        animator.Step();
        REQUIRE(currAnimation->GetState() == AnimationStateExternal::Ended);
        REQUIRE(dynamic_cast<DummyAnimation *>(currAnimation)->id == 2);
    }

    SECTION("Add lower priority and end") {
        auto lifetime3 = animator.start_animations(DummyAnimation(3), 0);
        REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->id == 2);
        lifetime2.Stop();
        animator.Step();
        animator.Step();
        REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->id == 1);
        lifetime.Stop();
        animator.Step();
        animator.Step();
        REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->id == 3);
    }
}

TEST_CASE("Animator same priority", "[Animator]") {

    Animator<2> animator({ 0, 3 });
    REQUIRE(animator.get_current() == nullptr);
    REQUIRE(animator.get_next() == nullptr);

    DummyAnimation animation(1);

    auto lifetime = animator.start_animations<DummyAnimation>(animation, 1);
    REQUIRE(animator.get_current() == nullptr);
    REQUIRE(animator.get_next() != nullptr);
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_next())->id == 1);

    animator.Step();
    REQUIRE(animator.get_current() != nullptr);
    REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->stepCounter == 1);

    animator.Step();
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->stepCounter == 2);
    REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);

    DummyAnimation animation2(2);
    auto lifetime2 = animator.start_animations<DummyAnimation>(animation2, 1);
    REQUIRE(animator.get_next() != nullptr);
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_next())->id == 2);

    Animation *nextAnimation = animator.get_next();
    Animation *currAnimation = animator.get_current();
    REQUIRE(animator.get_current() != animator.get_next());
    REQUIRE(animator.get_next() != nullptr);

    animator.Step();
    REQUIRE(dynamic_cast<DummyAnimation *>(currAnimation)->stepCounter == 3);
    REQUIRE(animator.get_current() != animator.get_next());
    REQUIRE(currAnimation->GetState() == AnimationStateExternal::Ended);

    animator.Step();
    REQUIRE(animator.get_next() == nullptr);
    REQUIRE(animator.get_current() != animator.get_next());
    REQUIRE(animator.get_current() == nextAnimation);
    REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->stepCounter == 1);

    SECTION("stop in order") {

        lifetime2.Stop();
        REQUIRE(animator.get_next() != nullptr);
        REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);

        nextAnimation = animator.get_next();
        currAnimation = animator.get_current();

        animator.Step();
        REQUIRE(animator.get_next() != nullptr);
        REQUIRE(currAnimation->GetState() == AnimationStateExternal::Ended);
        REQUIRE(animator.get_next()->GetState() == AnimationStateExternal::Ended);
        REQUIRE(dynamic_cast<DummyAnimation *>(currAnimation)->id == 2);

        animator.Step();
        REQUIRE(animator.get_next() == nullptr);
        REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);
        REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->id == 1);

        lifetime.Stop();
        REQUIRE(animator.get_next() == nullptr);
        REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);
        currAnimation = animator.get_current();

        animator.Step();
        REQUIRE(currAnimation->GetState() == AnimationStateExternal::Ended);
        REQUIRE(animator.get_current() == nullptr);
    }

    SECTION("stop out of order") {
        lifetime.Stop();
        REQUIRE(animator.get_next() == nullptr);
        REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);

        animator.Step();
        REQUIRE(animator.get_next() == nullptr);
        REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->id == 2);

        animator.Step();
        REQUIRE(animator.get_next() == nullptr);
        REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);
        REQUIRE(dynamic_cast<DummyAnimation *>(animator.get_current())->id == 2);

        lifetime2.Stop();
        REQUIRE(animator.get_next() == nullptr);
        REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);
        currAnimation = animator.get_current();

        animator.Step();
        REQUIRE(currAnimation->GetState() == AnimationStateExternal::Ended);
        REQUIRE(dynamic_cast<DummyAnimation *>(currAnimation)->id == 2);
    }
}
TEST_CASE("Animator stopping") {
    Animator<2> animator({ 0, 3 });
    timer::time = 0;
    auto lifetime1 = animator.start_animations(DummyAnimation(1), 1);
    animator.Step();
    REQUIRE(animator.get_current() != nullptr);
    REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);
    animator.pause_animator();
    REQUIRE(animator.get_current() != nullptr);
    REQUIRE(animator.get_current()->GetState() == AnimationStateExternal::InProgress);
    Animation *currAnimation = animator.get_current();

    animator.Step();

    REQUIRE(animator.get_current() == nullptr);
    REQUIRE(currAnimation->GetState() == AnimationStateExternal::Ended);

    animator.Step();

    REQUIRE(animator.get_current() == nullptr);
    REQUIRE(currAnimation->GetState() == AnimationStateExternal::Ended);

    animator.start_animator();

    REQUIRE(animator.get_current() == nullptr);
    REQUIRE(animator.get_next() == currAnimation);
    REQUIRE(currAnimation->GetState() == AnimationStateExternal::Ended);

    animator.Step();
    REQUIRE(currAnimation->GetState() == AnimationStateExternal::InProgress);
    REQUIRE(animator.get_current() == currAnimation);

    animator.start_animator();

    REQUIRE(animator.get_current() == currAnimation);
    REQUIRE(currAnimation->GetState() == AnimationStateExternal::InProgress);
    REQUIRE(animator.get_next() == nullptr);
}

TEST_CASE("Running the animation normally", "[Animations,Fading]") {
    uint32_t period = 100;
    GetLeds().Reset();
    std::pair<uint16_t, uint16_t> leds = { 0, 2 };
    leds::Color color = leds::Color(255, 0, 0);
    timer::time = 0;
    Fading fading(color, period);

    CHECK_THAT(GetLeds(), MK4LedsMatcher());
    fading.Step(leds);
    CHECK_THAT(GetLeds(), MK4LedsMatcher());

    SECTION("Running the animation") {
        timer::time = period;
        fading.Step(leds);
        CHECK_THAT(GetLeds(), MK4LedsMatcher(color));

        timer::time = 2 * period;
        fading.Step(leds);
        CHECK_THAT(GetLeds(), MK4LedsMatcher());
    }
}
TEST_CASE("Stopping the animation", "[Animations,Fading]") {
    uint32_t period = 100;
    GetLeds().Reset();
    std::pair<uint16_t, uint16_t> leds = { 0, 2 };
    leds::Color color = leds::Color(0, 0, 0);
    timer::time = 0;
    Fading fading(color, period);
    CHECK_THAT(GetLeds(), MK4LedsMatcher());
    fading.Step(leds);
    CHECK_THAT(GetLeds(), MK4LedsMatcher());

    timer::time = period / 2;
    fading.EndAnimation();

    // stop the animation while it is rising
    REQUIRE(fading.GetState() == AnimationStateExternal::InProgress);
    fading.Step(leds);
    REQUIRE(fading.GetState() == AnimationStateExternal::InProgress);
    leds::Color halfIntensity = color * 0.5;
    CHECK_THAT(GetLeds(), MK4LedsMatcher(halfIntensity));

    timer::time += period;
    fading.Step(leds);
    CHECK_THAT(GetLeds(), MK4LedsMatcher(halfIntensity));
    REQUIRE(fading.GetState() == AnimationStateExternal::InProgress);

    SECTION("Stopping the animation") {
        // leds should be turned off
        timer::time += period / 2;
        fading.Step(leds);
        CHECK_THAT(GetLeds(), MK4LedsMatcher());
        REQUIRE(fading.GetState() == AnimationStateExternal::Ended);

        timer::time += period;
        CHECK_THAT(GetLeds(), MK4LedsMatcher());
        REQUIRE(fading.GetState() == AnimationStateExternal::Ended);
    }

    SECTION("Calling step when the animation should be ended") {
        timer::time += 2 * period;
        fading.Step(leds);
        CHECK_THAT(GetLeds(), MK4LedsMatcher());
        REQUIRE(fading.GetState() == AnimationStateExternal::Ended);

        timer::time += period;
        CHECK_THAT(GetLeds(), MK4LedsMatcher());
        REQUIRE(fading.GetState() == AnimationStateExternal::Ended);
    }
}

TEST_CASE("Solid color", "[Animation, Solid]") {
    timer::time = 0;
    leds::Color color(leds::HSV { 0, 100, 100 });
    SolidColor animation(color);
    std::pair<uint16_t, uint16_t> leds = { 0, 2 };
    animation.Step(leds);
    CHECK_THAT(GetLeds(), MK4LedsMatcher(color));
    timer::time += 100;
    animation.EndAnimation();
    CHECK_THAT(GetLeds(), MK4LedsMatcher(color));
    timer::time += 250;
    animation.Step(leds);
    CHECK_THAT(GetLeds(), MK4LedsMatcher(color * 0.5));
    timer::time += 250;
    animation.Step(leds);
    CHECK_THAT(GetLeds(), MK4LedsMatcher());
}

TEST_CASE("HSV to RGB ") {

    SECTION("RED") {
        leds::Color color(leds::HSV { 0, 100, 100 });
        REQUIRE(color.r == 255);
        REQUIRE(color.g == 0);
        REQUIRE(color.b == 0);
    }

    SECTION("GREEN") {
        leds::Color color(leds::HSV { 120, 100, 100 });
        REQUIRE(color.r == 0);
        REQUIRE(color.g == 255);
        REQUIRE(color.b == 0);
    }

    SECTION("BLUE") {
        leds::Color color(leds::HSV { 240, 100, 100 });
        REQUIRE(color.r == 0);
        REQUIRE(color.g == 0);
        REQUIRE(color.b == 255);
    }
}
