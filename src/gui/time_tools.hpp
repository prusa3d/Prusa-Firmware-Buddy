#pragma once

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
 * @brief Time zone offset in minutes.
 * @warning Never change these values. They are stored in config_store.
 */

enum class TimeOffsetMinutes : int8_t {
    _0min,
    _30min,
    _45min,
};

/**
 * @brief Time zone summertime offset.
 * @warning Never change these values. They are stored in config_store.
 */

/// @return current timezone offset in minutes as integer value (minutes * seconds)
int8_t get_current_timezone_minutes();

/// @param new_offset set timezone minute offset
void set_timezone_minutes_offset(TimeOffsetMinutes new_offset);

/// @return current timezone minutes offset
TimeOffsetMinutes get_timezone_minutes_offset();

enum class TimeOffsetSummerTime : int8_t {
    _wintertime,
    _summertime
};

/// @return current timezone summertime offset in hours
int8_t get_current_timezone_summertime();

/// @param new_offset set timezone summertime offset
void set_timezone_summertime_offset(TimeOffsetSummerTime new_offset);

/// @return current timezone summertime offset
TimeOffsetSummerTime get_timezone_summertime_offset();

/// @param new_format set time format
void set_time_format(TimeFormat new_format);

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
