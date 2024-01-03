#include "time_helper.hpp"
#include <ctime>

void format_duration(std::span<char> buffer, std::uint32_t duration, bool display_seconds) {
    static constexpr std::uint32_t SECONDS_PER_DAY = 24 * 60 * 60;
    time_t time = (time_t)duration;

    const struct tm *timeinfo = gmtime(&time);
    [[maybe_unused]] int count = 0;
    if (timeinfo->tm_yday) {
        // days are recalculated, because timeinfo shows number of days in year and we want more days than 365
        uint16_t days = duration / SECONDS_PER_DAY;
        count = snprintf(buffer.data(), buffer.size(), "%ud %uh", days, timeinfo->tm_hour);
    } else if (timeinfo->tm_hour) {
        count = snprintf(buffer.data(), buffer.size(), "%ih %2im", timeinfo->tm_hour, timeinfo->tm_min);
    } else if (display_seconds) {
        if (timeinfo->tm_min) {
            count = snprintf(buffer.data(), buffer.size(), "%im %2is", timeinfo->tm_min, timeinfo->tm_sec);
        } else {
            count = snprintf(buffer.data(), buffer.size(), "%is", timeinfo->tm_sec);
        }
    } else {
        count = snprintf(buffer.data(), buffer.size(), "%im", timeinfo->tm_min);
    }
    assert(count > 0);
    assert(static_cast<std::uint32_t>(count) < buffer.size());
}
