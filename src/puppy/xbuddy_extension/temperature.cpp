/// @file
#include "temperature.hpp"

#include <array>
#include <cstddef>
#include <algorithm>
#include <cmath>
#include <span>

// Thermistor: 104NT-4-R025H42G

namespace {
struct ThermistorTableRecord {
    int16_t temp;
    float r_kohm;
};

struct ThermistorLookupRecord {
    int16_t raw;
    int16_t temp;
};

template <size_t table_size>
consteval auto thermistor_lookup_table(uint16_t r_pullup, uint16_t raw_max, const std::array<ThermistorTableRecord, table_size> &table) {
    std::array<ThermistorLookupRecord, table_size> result;
    size_t i = 0;
    for (const auto &rec : table) {
        const float r = rec.r_kohm * 1000.0f;
        result[i++] = ThermistorLookupRecord {
            .raw = static_cast<int16_t>(std::round(r * raw_max / (r + r_pullup))),
            .temp = rec.temp,
        };
    }

    std::sort(result.begin(), result.end(), [](auto a, auto b) { return a.raw < b.raw; });
    return result;
}

constexpr float raw_to_celsius(uint16_t raw, const std::span<const ThermistorLookupRecord> &lookup) {
    // Limit to end - 2 so that we are guaranteed to have 2 samples to deduce the slope from
    const auto it = std::lower_bound(lookup.begin(), lookup.end() - 2, raw, [](auto a, auto val) { return a.raw < val; });
    const auto lo = *it;
    const auto hi = *(it + 1);

    return lo.temp + static_cast<float>(static_cast<int16_t>(raw) - lo.raw) * (hi.temp - lo.temp) / (hi.raw - lo.raw);
}

constexpr auto ntc_104nt_lookup = thermistor_lookup_table(4700, 4095,
    std::to_array<ThermistorTableRecord>({
        { -50, 8887 },
        { -30, 2156 },
        { -10, 623.2 },
        { 0, 354.6 },
        { 10, 208.8 },
        { 25, 100 },
        { 40, 50.9 },
        { 50, 33.45 },
        { 60, 22.48 },
        { 80, 10.8 },
        { 85, 9.094 },
        { 100, 5.569 },
        { 120, 3.058 },
    }));

} // namespace

// Some checks that we're calculating correctly
static_assert(ntc_104nt_lookup[6].temp == 40);
static_assert(ntc_104nt_lookup[6].raw == 3749);

static_assert(raw_to_celsius(0, ntc_104nt_lookup) > 120);
static_assert(std::abs(raw_to_celsius(2220, ntc_104nt_lookup) - 100) < 1);
static_assert(std::abs(raw_to_celsius(3386, ntc_104nt_lookup) - 60) < 1);
static_assert(std::abs(raw_to_celsius(3749, ntc_104nt_lookup) - 40) < 1);
static_assert(raw_to_celsius(5000, ntc_104nt_lookup) < -50);

float temperature::raw_to_celsius(uint16_t raw) {
    return raw_to_celsius(raw, ntc_104nt_lookup);
}
