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
