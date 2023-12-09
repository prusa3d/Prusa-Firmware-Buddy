#include "time_tools.hpp"
#include <config_store/store_instance.hpp>
#include <ctime>

namespace time_tools {

void set_time_format(TimeFormat new_format) {
    config_store().time_format.set(new_format);
}

TimeFormat get_time_format() {
    return config_store().time_format.get();
}

int8_t get_current_timezone_minutes() {
    bool neg = config_store().timezone.get() < 0;
    switch (config_store().timezone_minutes.get()) {
    case TimeOffsetMinutes::_0min:
        return 0;
    case TimeOffsetMinutes::_30min:
        if (neg) {
            return -30;
        } else {
            return 30;
        }
    case TimeOffsetMinutes::_45min:
        if (neg) {
            return -45;
        } else {
            return 45;
        }
    default:
        assert(0);
    }
    return 0;
}

void set_timezone_minutes_offset(TimeOffsetMinutes new_offset) {
    config_store().timezone_minutes.set(new_offset);
}

TimeOffsetMinutes get_timezone_minutes_offset() {
    return config_store().timezone_minutes.get();
}

int8_t get_current_timezone_summertime() {
    switch (config_store().timezone_summer.get()) {
    case TimeOffsetSummerTime::_summertime:
        return 1;
    case TimeOffsetSummerTime::_wintertime:
        return 0;
    default:
        assert(0);
        break;
    }
    return 0;
}

void set_timezone_summertime_offset(TimeOffsetSummerTime new_offset) {
    config_store().timezone_summer.set(new_offset);
}

TimeOffsetSummerTime get_timezone_summertime_offset() {
    return config_store().timezone_summer.get();
}

namespace {
    char text_buffer[] = "--:-- --"; ///< Buffer for time string, needs to fit "01:23 am"
    struct tm last_time = {}; ///< Last time used to print to text_buffer
    TimeFormat last_format = TimeFormat::_12h; ///< Last time format used to print to text_buffer
} // namespace

bool update_time() {
    time_t t = time(nullptr);
    if (t != (time_t)-1) { // Time is initialized in RTC (from sNTP)
        struct tm now;
        int8_t timezone_diff = config_store().timezone.get();
        int8_t timezone_summertime = get_current_timezone_summertime();
        int8_t timezone_min_diff = get_current_timezone_minutes();
        t += (timezone_diff + timezone_summertime) * 3600;
        t += timezone_min_diff * 60;
        localtime_r(&t, &now);

        TimeFormat current_format = config_store().time_format.get();

        if (!(last_time.tm_hour == now.tm_hour && last_time.tm_min == now.tm_min) // Time (hour || minute) has changed from previous call
            || current_format != last_format) { // Time format has changed from previous call
            last_time = now;
            last_format = current_format;
            if (current_format == TimeFormat::_24h) {
                strftime(text_buffer, std::size(text_buffer), "%H:%M", &now);
            } else {
                strftime(text_buffer, std::size(text_buffer), "%I:%M %p", &now);
            }
            return true; // Time changed and was printed
        }
    }

    return false;
}

const char *get_time() {
    return text_buffer;
}

} // namespace time_tools
