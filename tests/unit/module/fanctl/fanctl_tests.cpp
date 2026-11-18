#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

#include <fanctl/CFanCtl3Wire.hpp>
#include <fanctl/dynamic_pwm.hpp>

namespace {

// The fan controllers run tick() from a 1 kHz timer interrupt.
constexpr uint32_t ticks_per_second = 1000;

CFanCtlPWM::PWMParams pwm_params(uint8_t max_value, uint8_t phase_shift_threshold = 0) {
    return {
        .pwm_min = 0,
        .pwm_max = max_value,
        .phase_shift_threshold = phase_shift_threshold,
        .has_inverted_pwm = false,
    };
}

/// A 3-wire fan at a nearly constant speed. The tach output is valid only while the fan has power
/// (the PWM switches the fan supply). Without power, the pull-up keeps it high. CFanCtlTach
/// assumes 4 tach edges for each revolution. A small phase wobble (+-0.5 edge) keeps the model
/// from locking to the PWM period, as a real fan does not lock to it either.
struct SimulatedFan {
    double rpm;

    bool tach(uint32_t tick, bool powered) const {
        if (!powered) {
            return true;
        }
        const double edges = rpm / 15.0 / ticks_per_second * tick + 0.5 * std::sin(2 * std::numbers::pi * tick / 3700.0);
        return static_cast<int64_t>(std::floor(edges)) % 2;
    }
};

/// The state that the pin functions use. inplace_function has a small capture size, so the
/// lambdas capture only a pointer to this.
struct Pins {
    bool powered = false;
    uint32_t tick = 0;
    SimulatedFan fan { 0 };
};

} // namespace

TEST_CASE("fanctl: dynamic PWM period", "[fanctl]") {
    CHECK(fanctl::dynamic_pwm_period(1) == 250);
    CHECK(fanctl::dynamic_pwm_period(49) == 250);
    CHECK(fanctl::dynamic_pwm_period(50) == 125);
    CHECK(fanctl::dynamic_pwm_period(79) == 125);
    CHECK(fanctl::dynamic_pwm_period(80) == 50);
    CHECK(fanctl::dynamic_pwm_period(119) == 50);
    CHECK(fanctl::dynamic_pwm_period(120) == 40);
    CHECK(fanctl::dynamic_pwm_period(255) == 40);
}

TEST_CASE("CFanCtlPWM: on for exactly the PWM value, and pwm_on is as documented", "[fanctl]") {
    // The periods that dynamic PWM uses. 50 is also the default print fan period.
    const int max_value = GENERATE(40, 50, 125, 250);

    for (int val = 0; val <= max_value; ++val) {
        CAPTURE(max_value, val);
        bool pin = false;
        CFanCtlPWM pwm([p = &pin](bool v) { *p = v; }, pwm_params(static_cast<uint8_t>(max_value)));
        pwm.set_PhaseShiftMode(CFanCtlPWM::none);
        pwm.set_PWM(static_cast<uint8_t>(val));

        // The new value takes effect at the start of the next period.
        for (int t = 0; t < max_value; ++t) {
            pwm.tick();
        }

        for (int period = 0; period < 3; ++period) {
            int on_ticks = 0;
            for (int t = 0; t < max_value; ++t) {
                const int16_t pwm_on = pwm.tick();
                // "positive number means pwm is on N ticks, negative number means pwm is off
                // and will be switched on in -N ticks"
                const int expected = (t < val) ? t : t - max_value;
                CAPTURE(t, pwm_on);
                REQUIRE(pwm_on == expected);
                REQUIRE(pin == (pwm_on >= 0));
                on_ticks += pin;
            }
            REQUIRE(on_ticks == val);
        }
    }
}

TEST_CASE("CFanCtlPWM: phase shifting keeps the on time of each period", "[fanctl]") {
    struct Case {
        int max_value;
        int phase_shift_threshold;
        int val;
    };
    const Case c = GENERATE(
        Case { 50, 20, 2 }, Case { 50, 20, 5 }, Case { 50, 20, 10 }, Case { 50, 20, 20 },
        // A long period: max_value - val does not fit in int8_t.
        Case { 250, 100, 37 }, Case { 250, 100, 100 });
    const auto mode = GENERATE(CFanCtlPWM::triangle, CFanCtlPWM::random);
    CAPTURE(c.max_value, c.val, static_cast<int>(mode));

    bool pin = false;
    CFanCtlPWM pwm([p = &pin](bool v) { *p = v; }, pwm_params(static_cast<uint8_t>(c.max_value), static_cast<uint8_t>(c.phase_shift_threshold)));
    pwm.set_PhaseShiftMode(mode);
    pwm.set_PWM(static_cast<uint8_t>(c.val));
    for (int t = 0; t < c.max_value; ++t) {
        pwm.tick();
    }

    for (int period = 0; period < 20; ++period) {
        int on_ticks = 0;
        for (int t = 0; t < c.max_value; ++t) {
            const int16_t pwm_on = pwm.tick();
            REQUIRE(pin == (pwm_on >= 0));
            on_ticks += pin;
        }
        REQUIRE(on_ticks == c.val);
    }
}

TEST_CASE("CFanCtlPWM: a change of the period in the middle of a period", "[fanctl]") {
    // CFanCtl3WireDynamic changes max_value at any time, also to a value below the counter.
    bool pin = false;
    CFanCtlPWM pwm([p = &pin](bool v) { *p = v; }, pwm_params(250));
    pwm.set_PhaseShiftMode(CFanCtlPWM::none);
    pwm.set_PWM(100);
    for (int t = 0; t < 250 + 200; ++t) {
        pwm.tick();
    }

    pwm.set_max_PWM(40);
    pwm.set_PWM(20);
    // The rest of the old period ends at once, then one period loads the new value.
    for (int t = 0; t < 1 + 40; ++t) {
        pwm.tick();
    }
    for (int period = 0; period < 3; ++period) {
        int on_ticks = 0;
        for (int t = 0; t < 40; ++t) {
            const int16_t pwm_on = pwm.tick();
            REQUIRE(pin == (pwm_on >= 0));
            on_ticks += pin;
        }
        REQUIRE(on_ticks == 20);
    }
}

TEST_CASE("CFanCtlTach: measured RPM with the PWM switching the fan supply", "[fanctl]") {
    struct Case {
        int max_value; ///< PWM period in ticks
        int val; ///< on ticks in each period
    };
    // The periods and values that dynamic PWM uses at print fan speeds from 10% to 100%, and the
    // fixed 50-tick period of CFanCtl3Wire at 20% and 25%.
    const Case c = GENERATE(
        Case { 250, 24 }, Case { 250, 39 }, Case { 250, 48 }, // below 50/255
        Case { 125, 24 }, Case { 125, 31 }, Case { 125, 38 }, // 50-79
        Case { 50, 15 }, Case { 50, 20 }, // 80-119
        Case { 40, 18 }, Case { 40, 40 }, // 120 and more
        Case { 50, 10 }, Case { 50, 12 }); // fixed period
    const double rpm = GENERATE(1537.3, 3311.7, 5523.9);
    CAPTURE(c.max_value, c.val, rpm);

    Pins s { .fan = { rpm } };
    CFanCtlPWM pwm([p = &s](bool v) { p->powered = v; }, pwm_params(static_cast<uint8_t>(c.max_value)));
    pwm.set_PhaseShiftMode(CFanCtlPWM::none);
    pwm.set_PWM(static_cast<uint8_t>(c.val));
    CFanCtlTach tach([p = &s]() { return p->fan.tach(p->tick, p->powered); });

    // The tach ignores the first two ticks of each PWM pulse, but it counts them as on time.
    // Thus the measured value is low by about 2 / val: 5% at 100%, 20% at 10 of 50 ticks.
    // The self test RPM ranges were set with this measurement, so this test keeps it.
    const double expected = rpm * (c.val - 2) / c.val;
    // Resolution: one tach edge more or less in a 1 s window changes the value by 15 RPM, scaled by
    // 1 s / on time. At 24 of 250 ticks, that is about 156 RPM, as large as FANCTLPRINT_RPM_MIN.
    const double on_ticks_per_second = static_cast<double>(c.val) * ticks_per_second / c.max_value;
    const double step = 15.0 * ticks_per_second / on_ticks_per_second;
    CAPTURE(expected, step);

    // The RPM filter takes 3/4 of each new 1 s value. After 3 s, the start value has little effect.
    // The firmware gives an error on any single value, so check all values, not only the last.
    uint16_t lowest = UINT16_MAX;
    uint16_t highest = 0;
    for (; s.tick < 15 * ticks_per_second; ++s.tick) {
        tach.tick(pwm.tick());
        if (s.tick >= 3 * ticks_per_second) {
            lowest = std::min(lowest, tach.getRPM());
            highest = std::max(highest, tach.getRPM());
        }
    }
    // Margin: up to 4 edge steps from the window quantization and the filter, and 2% from the
    // phase wobble of the model. The int8_t pwm_on bug read 21-42% of the real speed here.
    CHECK(lowest > expected - 4 * step - 0.02 * rpm);
    CHECK(highest < rpm + 4 * step + 0.02 * rpm);
}

namespace {

constexpr uint16_t min_rpm = 150; // FANCTLPRINT_RPM_MIN on the CORE One

CFanCtl3Wire::FanParams fan_params(uint8_t max_pwm) {
    return {
        .min_pwm = static_cast<uint8_t>(max_pwm / 10),
        .max_pwm = max_pwm,
        .min_rpm = min_rpm,
        .max_rpm = 6850,
        .thr_pwm = static_cast<uint8_t>(max_pwm * 4 / 10),
        .autofan = is_autofan_t::no,
        .skip_tacho = skip_tacho_t::no,
        .min_pwm_to_measure_rpm = 0,
        .has_inverted_pwm = false,
    };
}

struct FanUnderTest {
    Pins s;
    CFanCtl3Wire ctl;

    FanUnderTest(double rpm, uint8_t max_pwm)
        : s { .fan = { rpm } }
        , ctl {
            [p = &s](bool v) { p->powered = v; },
            [p = &s]() { return p->fan.tach(p->tick, p->powered); },
            fan_params(max_pwm)
        } {}

    /// Runs \p ticks ticks. Returns the number of ticks with the fan powered.
    uint32_t run(uint32_t ticks) {
        uint32_t on = 0;
        for (uint32_t i = 0; i < ticks; ++i, ++s.tick) {
            ctl.tick();
            on += s.powered;
        }
        return on;
    }

    /// Runs \p ticks ticks. Returns true if is_fan_ok() was true on every tick.
    bool run_fan_ok(uint32_t ticks) {
        bool ok = true;
        for (uint32_t i = 0; i < ticks; ++i, ++s.tick) {
            ctl.tick();
            ok &= ctl.is_fan_ok();
        }
        return ok;
    }
};

} // namespace

TEST_CASE("CFanCtl3Wire: start, run and stall", "[fanctl]") {
    FanUnderTest t(3000.0, 50);
    REQUIRE(t.ctl.get_state() == CFanCtlCommon::idle);

    // 25%: 12 of 50 ticks.
    t.ctl.set_pwm(64);
    t.run(1);
    REQUIRE(t.ctl.get_state() == CFanCtlCommon::starting);

    SECTION("the start uses full power until the tach shows rotation") {
        // A new PWM value takes effect at the start of the next period. Thus the full power
        // starts after the current period (50 ticks here, 250 ticks at 4 Hz dynamic PWM).
        CHECK(t.run(49) == 0);
        // FANCTL_START_EDGES edges at 3000 RPM (200 edges/s) take about 20 ms.
        const uint32_t on = t.run(50);
        CHECK(t.ctl.get_state() == CFanCtlCommon::rpm_stabilization);
        CHECK(on >= 25);
    }

    SECTION("a running fan has an RPM in range") {
        t.run(FANCTL_RPM_DELAY + 3 * ticks_per_second);
        REQUIRE(t.ctl.get_state() == CFanCtlCommon::running);
        CHECK(t.ctl.get_rpm_is_ok());
        // 12 of 50 ticks: the measurement is about 2/12 low (see the CFanCtlTach test).
        CHECK(t.ctl.get_actual_rpm() > 0.95 * t.s.fan.rpm * 10 / 12);
        // The set PWM, not full power: 12 of 50 ticks.
        CHECK(t.run(50 * 20) == 12 * 20);
    }

    SECTION("a stopped fan gives error_running, and the PWM stays at the set value") {
        t.run(FANCTL_RPM_DELAY + 3 * ticks_per_second);
        REQUIRE(t.ctl.get_state() == CFanCtlCommon::running);

        t.s.fan.rpm = 0;
        t.run(5 * ticks_per_second);
        CHECK(t.ctl.get_state() == CFanCtlCommon::error_running);
        CHECK_FALSE(t.ctl.get_rpm_is_ok());
        CHECK_FALSE(t.ctl.is_fan_ok());

        // Current behaviour: the error state does not start the fan again at full power.
        // A fan that stopped at a low PWM thus stays stopped until the PWM goes to 0 and back.
        CHECK(t.run(50 * 20) == 12 * 20);
    }
}

TEST_CASE("CFanCtl3Wire: a slow fan at a low PWM with a 250-tick period", "[fanctl]") {
    // Dynamic PWM uses the 250-tick period below 50/255. The CORE One PC filament presets set
    // min_fan_speed 15% (M106 S38). A fan that turns slowly there must not give a false error.
    const double rpm = GENERATE(500.0, 700.0, 1000.0);
    CAPTURE(rpm);
    FanUnderTest t(rpm, 250);
    t.ctl.set_pwm(38);
    t.run(FANCTL_RPM_DELAY + 5 * ticks_per_second);
    REQUIRE(t.ctl.get_state() == CFanCtlCommon::running);
    CHECK(t.run_fan_ok(20 * ticks_per_second));
}
