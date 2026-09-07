#include "time_ticks.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>
#include <random>

// ── helpers ──────────────────────────────────────────────────────────────────

/// Half an LSB in µs (2^-17 µs), the documented boundary window width.
static constexpr double HALF_LSB_US = 0.5 / (1 << 16);

// ── 1. Round-trips ────────────────────────────────────────────────────────────

TEST_CASE("TimeTicks round-trip from_us/to_us_floor") {
    // Exact: from_us shifts left 16, to_us_floor shifts right 16.
    for (int64_t us : { 0LL, 1LL, -1LL, 1'000'000LL, -1'000'000LL, 1'000'000'000'000LL }) {
        CAPTURE(us);
        CHECK(TimeTicks::from_us(us).to_us_floor() == us);
    }
}

TEST_CASE("TimeTicks round-trip from_seconds(double)/to_seconds within half LSB") {
    // to_seconds() = double(raw) / PER_SEC
    // from_seconds(double) = llround(sec * PER_SEC)
    // The round-trip error is |sec - double(llround(sec*PER_SEC))/PER_SEC| ≤ 0.5/PER_SEC
    const double half_lsb_s = 0.5 / TimeTicks::PER_SEC;
    for (double sec : { 0.0, 1.0, -1.0, 100.0, -100.0, 1e7, 3600.0, 0.001, 1e-6 }) {
        CAPTURE(sec);
        double rt = TimeTicks::from_seconds(sec).to_seconds();
        CHECK(std::abs(rt - sec) <= half_lsb_s);
    }
}

// ── 2. Truncation-compat sweep ───────────────────────────────────────────────
//
// from_seconds(x).to_us_floor() and int64_t(x*1e6) differ by at most 1 µs.
// Outside the documented ±½-LSB boundary window the two agree; inside (when
// x·1e6 falls within 2^-17 µs below a whole number) they may differ by 1
// because llround rounds up while truncation stays below.

TEST_CASE("TimeTicks truncation-compat: difference at most 1 µs, random inputs") {
    std::mt19937_64 rng(0xDEAD'BEEF'CAFE'1234ULL);
    std::uniform_real_distribution<double> sec_dist(-1000.0, 1000.0);
    for (int i = 0; i < 100'000; ++i) {
        double sec = sec_dist(rng);
        int64_t trunc_formula = static_cast<int64_t>(sec * 1e6);
        int64_t tick_formula = TimeTicks::from_seconds(sec).to_us_floor();
        int64_t diff = tick_formula - trunc_formula;
        CAPTURE(sec, trunc_formula, tick_formula);
        CHECK((diff == 0 || diff == 1 || diff == -1));
    }
}

TEST_CASE("TimeTicks truncation-compat: non-negative times with clean fractional part") {
    // For non-negative inputs (the realistic firmware domain for absolute timestamps),
    // trunc == floor, so differences occur only in the documented ±½-LSB boundary window.
    //
    // n + 0.25 µs: fractional part well above the 2^-17 boundary — both formulas give n.
    for (int64_t n : { int64_t(0), int64_t(1), int64_t(100), int64_t(1'000'000) }) {
        double sec = (static_cast<double>(n) + 0.25) * 1e-6;
        int64_t trunc_formula = static_cast<int64_t>(sec * 1e6);
        int64_t tick_formula = TimeTicks::from_seconds(sec).to_us_floor();
        CAPTURE(n, sec, trunc_formula, tick_formula);
        CHECK(trunc_formula == tick_formula);
        CHECK(trunc_formula == n);
    }
    // Near the boundary window: n - epsilon where epsilon < 2^-17 µs.
    // The two formulas may differ by 1 (llround rounds up; trunc stays below).
    for (int64_t n : { int64_t(1), int64_t(100), int64_t(1'000'000) }) {
        double sec_near = (static_cast<double>(n) - HALF_LSB_US * 0.1) * 1e-6;
        int64_t trunc_near = static_cast<int64_t>(sec_near * 1e6);
        int64_t tick_near = TimeTicks::from_seconds(sec_near).to_us_floor();
        CAPTURE(n, sec_near, trunc_near, tick_near);
        // Difference is ≤ 1; inside the window tick_near may be n while trunc_near is n-1.
        int64_t d = tick_near - trunc_near;
        CHECK((d == 0 || d == 1 || d == -1));
    }
}

// ── 3. Exact accumulation vs double drift ────────────────────────────────────

TEST_CASE("TimeTicks exact accumulation: zero tick drift over 1M segments") {
    // Synthetic move durations as double seconds with fractional-µs content.
    // Fixed seed for reproducibility.
    std::mt19937_64 rng(0x1234'5678'9ABC'DEF0ULL);
    std::uniform_real_distribution<double> dur_dist(100e-6, 2.0); // 100 µs – 2 s

    const int N = 1'000'000;

    // Ground truth: sum of llround(d * PER_SEC) accumulated in int64.
    int64_t exact_raw = 0;
    // TimeTicks accumulator (equivalent of print_time += move_time).
    TimeTicks tick_sum = TimeTicks::zero();
    // Float reference to demonstrate drift (single-precision).
    float float_sum_s = 0.0f;
    // High-precision reference for bounded-error check.
    long double hd_sum_s = 0.0L;

    for (int i = 0; i < N; ++i) {
        double dur_s = dur_dist(rng);
        int64_t dur_raw = llround(dur_s * TimeTicks::PER_SEC);
        exact_raw += dur_raw;
        tick_sum += TimeTicks::from_seconds(dur_s);
        float_sum_s += static_cast<float>(dur_s);
        hd_sum_s += static_cast<long double>(dur_s);

        // (a) Tick accumulator must match the independently computed integer sum at every step.
        REQUIRE(tick_sum.raw() == exact_raw);

        // (b) Absolute error vs high-precision reference is bounded at every step.
        // Each from_seconds rounds by at most ½ LSB; i+1 additions accumulate at most (i+1) × ½ LSB.
        long double hd_raw_i = hd_sum_s * static_cast<long double>(TimeTicks::PER_SEC);
        long double error_raw_i = fabsl(static_cast<long double>(tick_sum.raw()) - hd_raw_i);
        REQUIRE(error_raw_i <= static_cast<long double>(i + 1) * 0.5L);
    }

    // (c) Float reference visibly drifts (documents the failure mode).
    // With 1M additions of 0.1–2 s values, float32 (24-bit mantissa) loses precision.
    long double hd_raw = hd_sum_s * static_cast<long double>(TimeTicks::PER_SEC);
    long double float_raw = static_cast<long double>(float_sum_s) * static_cast<long double>(TimeTicks::PER_SEC);
    long double float_error_raw = fabsl(float_raw - hd_raw);
    // Require at least 1 µs of drift (= PER_US raw units).
    INFO("Float drift in raw units: " << float_error_raw);
    CHECK(float_error_raw >= static_cast<long double>(TimeTicks::PER_US));
}

// ── 4. Delta invariant ───────────────────────────────────────────────────────

TEST_CASE("TimeTicks delta invariant: sum of floor-deltas == floor of final absolute") {
    // Σ(to_us_floor(t_i) - to_us_floor(t_{i-1})) == to_us_floor(t_final)
    // where t_0 = 0 and each segment is added as a from_seconds(double) duration
    // with fractional-µs content to exercise the sub-µs bits.
    std::mt19937_64 rng(0xFEED'FACE'DEAD'BEEFULL);
    std::uniform_real_distribution<double> dur_dist(100e-6, 0.5); // 100 µs – 500 ms

    const int N = 10'000;

    TimeTicks absolute = TimeTicks::zero();
    int64_t sum_of_deltas = 0;
    int64_t prev_floor = 0;

    for (int i = 0; i < N; ++i) {
        double dur_s = dur_dist(rng);
        absolute += TimeTicks::from_seconds(dur_s);
        int64_t cur_floor = absolute.to_us_floor();
        sum_of_deltas += cur_floor - prev_floor;
        prev_floor = cur_floor;
    }

    CHECK(sum_of_deltas == absolute.to_us_floor());
}

// ── 5. scaled_by, operator*, negatives, comparisons ─────────────────────────

TEST_CASE("TimeTicks arithmetic — scaled_by") {
    // sampling_rate of 1 ms → raw = 1000 * 2^16 = 65'536'000.
    TimeTicks sr = TimeTicks::from_us(1000);
    // 65'536'000 < 2^32 so debug assert passes.
    TimeTicks half = sr.scaled_by(0.5f);
    // Result should be 500 µs ± a few raw units (float precision).
    int64_t half_us = half.to_us_floor();
    CHECK(half_us >= 499);
    CHECK(half_us <= 500);

    // Negative ratio.
    TimeTicks neg_half = sr.scaled_by(-0.5f);
    CHECK(neg_half.to_us_floor() <= -499);
    CHECK(neg_half.to_us_floor() >= -501);
}

TEST_CASE("TimeTicks arithmetic — operator*") {
    TimeTicks sr = TimeTicks::from_us(1000); // 1 ms
    TimeTicks ten = sr * int64_t(10);
    CHECK(ten.to_us_floor() == 10'000);

    // Commuted form.
    TimeTicks ten2 = int64_t(10) * sr;
    CHECK(ten2.to_us_floor() == 10'000);
}

TEST_CASE("TimeTicks arithmetic — addition and subtraction") {
    TimeTicks a = TimeTicks::from_us(1'000'000); // 1 s
    TimeTicks b = TimeTicks::from_us(500'000); // 0.5 s
    CHECK((a + b).to_us_floor() == 1'500'000);
    CHECK((a - b).to_us_floor() == 500'000);

    TimeTicks c = a;
    c += b;
    CHECK(c.to_us_floor() == 1'500'000);
    c -= b;
    CHECK(c.to_us_floor() == 1'000'000);
}

TEST_CASE("TimeTicks arithmetic — unary negation") {
    TimeTicks a = TimeTicks::from_us(1000);
    CHECK((-a).to_us_floor() == -1000);
    CHECK((-TimeTicks::zero()).to_us_floor() == 0);
}

TEST_CASE("TimeTicks to_us_floor floors negatives") {
    // floor(-0.5 µs) should be -1 (arithmetic shift floors toward −∞).
    // -0.5 µs in raw = -(2^16 / 2) = -32768.
    TimeTicks neg_half_us = TimeTicks::from_raw(-32768);
    CHECK(neg_half_us.to_us_floor() == -1);

    // -1 µs exactly → -1.
    CHECK(TimeTicks::from_us(-1).to_us_floor() == -1);

    // -1.5 µs in raw = -(3 * 2^15) = -98304.
    TimeTicks neg_one_and_half = TimeTicks::from_raw(-98304);
    CHECK(neg_one_and_half.to_us_floor() == -2);
}

TEST_CASE("TimeTicks comparisons and sentinel ordering") {
    TimeTicks zero = TimeTicks::zero();
    TimeTicks one_ms = TimeTicks::from_us(1000);
    TimeTicks neg_one_ms = TimeTicks::from_us(-1000);
    TimeTicks mx = TimeTicks::max();
    TimeTicks mn = TimeTicks::min();

    CHECK(zero < one_ms);
    CHECK(neg_one_ms < zero);
    CHECK(mn < neg_one_ms);
    CHECK(one_ms < mx);
    CHECK(mn < mx);

    // Equality.
    CHECK(zero == TimeTicks::zero());
    CHECK(mx == TimeTicks::max());
    CHECK(mn == TimeTicks::min());
    CHECK(zero != one_ms);

    // max() is above any real value; min() is below any real value.
    TimeTicks huge = TimeTicks::from_us(int64_t(1) << 45); // near range limit but valid
    CHECK(huge < mx);
    TimeTicks neg_huge = TimeTicks::from_us(-(int64_t(1) << 45));
    CHECK(mn < neg_huge);
}

// ── 6. PA sampling identity ──────────────────────────────────────────────────
//
// Proves: floor(floor(t_us)/n) == floor(t/n) when n is integer µs.
// This validates that integer-µs-division in pressure_advance is equivalent
// to direct fixed-point division.

TEST_CASE("TimeTicks PA sampling identity: floor(floor(t_us)/n) == floor(t_raw/(n*PER_US))") {
    std::mt19937_64 rng(0xABCD'EF01'2345'6789ULL);
    // t: 0 to 10 seconds, sub-µs precision.
    std::uniform_int_distribution<int64_t> raw_dist(0, int64_t(10'000'000) * TimeTicks::PER_US);
    // n: 1–5000 µs (representative sampling rates: 100 µs – 5 ms).
    std::uniform_int_distribution<int64_t> n_dist(1, 5000);

    for (int i = 0; i < 200'000; ++i) {
        int64_t raw = raw_dist(rng);
        int64_t n = n_dist(rng);
        TimeTicks t = TimeTicks::from_raw(raw);

        // LHS: floor(to_us_floor(t) / n)  — integer-µs path.
        int64_t lhs = t.to_us_floor() / n;

        // RHS: floor(t / (n * PER_US)) in raw units.
        // raw >= 0 so integer truncation equals floor.
        int64_t n_raw = n * TimeTicks::PER_US;
        int64_t rhs = raw / n_raw;

        CAPTURE(raw, n, lhs, rhs);
        CHECK(lhs == rhs);
    }
}
