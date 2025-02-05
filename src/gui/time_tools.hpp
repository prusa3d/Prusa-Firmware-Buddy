#pragma once

#include <cstdint>
#include <ctime>
#include <utility_extensions.hpp>

namespace time_tools {

/**
 * @brief Time format enum.
 * @warning Never change these values. They are stored in config_store.
 */
enum class TimeFormat : uint8_t {
    _12h,
    _24h,
};

/**
 * @brief Time zone offset in minutes. Adds the amount of minutes in + timezones, removes in -.
 * @warning Never change these values. They are stored in config_store.
 */
enum class TimezoneOffsetMinutes : uint8_t {
    no_offset,
    min30,
    min45,
    _cnt
};

/// Mapping of TimezoneOffsetMinutes -> actual amount of minutes
/// !!! This value has to be negated for <0 timezones
constexpr std::array<int8_t, ftrstd::to_underlying(TimezoneOffsetMinutes::_cnt)> timezone_offset_minutes_value = {
    0, 30, 45
};

enum class TimezoneOffsetSummerTime : uint8_t {
    no_summertime, ///< No offset
    summertime, ///< +1 hour
    _cnt
};

/// Calculates total timezone offset.
/// This takes into consideration all timezone configs – timezone, minutes offset, summertime
/// @returns the calculated offset in minutes, to be applied on the UTC standard time
int32_t calculate_total_timezone_offset_minutes();

/// @return current time format
TimeFormat get_time_format();

/**
 * @brief Update time string with current time.
 * @warning Not thread safe. Use only from GUI thread.
 * @return true if time string has changed
 */
bool update_time();

/**
 * @brief Access to printed time string.
 * @warning Not thread safe. Use only from GUI thread.
 * This string is updated by update_time() method.
 * @return string with printed time
 */
const char *get_time();

} // namespace time_tools
