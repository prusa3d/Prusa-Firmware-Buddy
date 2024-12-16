#include "time_tools.hpp"
#include <config_store/store_instance.hpp>

namespace time_tools {

TimeFormat get_time_format() {
    return config_store().time_format.get();
}

void set_timezone_minutes_offset(TimezoneOffsetMinutes new_offset) {
    config_store().timezone_minutes.set(new_offset);
}

TimezoneOffsetMinutes get_timezone_minutes_offset() {
    return config_store().timezone_minutes.get();
}

void set_timezone_summertime_offset(TimezoneOffsetSummerTime new_offset) {
    config_store().timezone_summer.set(new_offset);
}

TimezoneOffsetSummerTime get_timezone_summertime_offset() {
    return config_store().timezone_summer.get();
}

int32_t calculate_total_timezone_offset_minutes() {
    const int8_t timezone = config_store().timezone.get();
    const TimezoneOffsetMinutes timezone_minutes = config_store().timezone_minutes.get();

    static_assert(ftrstd::to_underlying(TimezoneOffsetSummerTime::no_summertime) == 0);
    static_assert(ftrstd::to_underlying(TimezoneOffsetSummerTime::summertime) == 1);

    return //
        static_cast<int32_t>(timezone) * 60

        // Minutes timezone, through lookup table, clamped for memory safety
        + ((timezone < 0) ? -1 : 1) * timezone_offset_minutes_value[std::min(static_cast<size_t>(timezone_minutes), timezone_offset_minutes_value.size() - 1)] //

        // Summer/wintertime. Summertime = +1, so we can simply do a static cast
        + static_cast<int32_t>(config_store().timezone_summer.get()) * 60;
}

namespace {
    char text_buffer[] = "--:-- --"; ///< Buffer for time string, needs to fit "01:23 am"
    struct tm last_time = {}; ///< Last time used to print to text_buffer
    TimeFormat last_format = TimeFormat::_12h; ///< Last time format used to print to text_buffer
} // namespace

time_t get_local_time() {
    time_t t = time(nullptr);

    // Check if time is initialized in RTC (from sNTP)
    if (t == invalid_time) {
        return invalid_time;
    }

    t += calculate_total_timezone_offset_minutes() * 60;

    return t;
}

bool update_time() {
    time_t t = get_local_time();
    if (t == invalid_time) {
        return false;
    }

    struct tm now;
    localtime_r(&t, &now);

    const TimeFormat current_format = config_store().time_format.get();

    // Check if anything has changed from the previous call
    if (last_time.tm_hour == now.tm_hour && last_time.tm_min == now.tm_min && current_format == last_format) {
        return false;
    }

    last_time = now;
    last_format = current_format;

    const char *format_str = (current_format == TimeFormat::_24h) ? "%H:%M" : "%I:%M %p";
    strftime(text_buffer, std::size(text_buffer), format_str, &now);

    // Time changed and was printed
    return true;
}

const char *get_time() {
    return text_buffer;
}

} // namespace time_tools
