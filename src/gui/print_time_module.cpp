// print_time_module.cpp

#include "format_print_will_end.hpp"
#include <ctime>
#include "marlin_client.hpp"
#include "print_time_module.hpp"
#include "time_tools.hpp"
#include "time_helper.hpp"
#include <config_store/store_instance.hpp>
#include <guiconfig/guiconfig.h>

PT_t PrintTime::update_loop(PT_t screen_format, window_text_t *out_print_end, [[maybe_unused]] window_text_t *out_print_dur) {
    // TODO: for MINI - Add time_dur condition
    // TODO: Non-context time <-> context time
    const uint32_t time_to_end = marlin_vars().time_to_end;

#if HAS_MINI_DISPLAY()
    if (out_print_dur) {
        const time_t rawtime = (time_t)marlin_vars().print_duration; // print_duration holds SECONDS
        if (rawtime != last_print_duration) {
            out_print_dur->SetTextColor(generate_duration(rawtime));
            out_print_dur->SetText(string_view_utf8::MakeRAM((const uint8_t *)text_time_dur.data()));
            out_print_dur->Invalidate();
        }
        last_print_duration = rawtime;
    }
#endif

    if (screen_format != PT_t::init && time_to_end != marlin_server::TIME_TO_END_INVALID && time_to_end == last_time_to_end) {
        return time_end_format;
    }

    const Color print_end_text_color = [&] {
        // Invalid
        if (time_to_end == marlin_server::TIME_TO_END_INVALID || time_to_end > 60 * 60 * 24 * 365) {
            strlcpy(text_time_end.data(), "N/A", MAX_END_TIMESTAMP_SIZE);
            return GuiDefaults::COLOR_VALUE_INVALID;
        }

        // Countdown (unknown wall time or in a short time)
        const time_t curr_sec = time(nullptr);
        if (curr_sec == -1 || time_to_end < COUNTDOWN_TIME_S) {
            time_end_format = PT_t::countdown;
            generate_countdown_string(time_to_end);
            return GuiDefaults::COLOR_VALUE_VALID;
        }

        // Timestamp
        const time_t local_cur_sec = curr_sec + time_tools::calculate_total_timezone_offset_minutes() * 60;
        generate_timestamp_string(local_cur_sec, time_to_end);

        time_end_format = PT_t::timestamp;

        // Add unknown marker
        if (marlin_vars().print_speed != 100) {
            strlcat(text_time_end.data(), "?", MAX_END_TIMESTAMP_SIZE);
        }

        return GuiDefaults::COLOR_VALUE_VALID;
    }();
    out_print_end->SetTextColor(print_end_text_color);
    out_print_end->SetText(string_view_utf8::MakeRAM((const uint8_t *)text_time_end.data()));
    out_print_end->Invalidate();
    last_time_to_end = time_to_end;

    return time_end_format;
}

void PrintTime::print_formatted_duration(uint32_t duration, std::span<char> buffer, bool parse_seconds) {
    // standard would be:
    // strftime(array.data(), array.size(), "%jd %Hh", timeinfo);
    format_duration(buffer, duration, parse_seconds);
}

void PrintTime::generate_countdown_string(const uint32_t time_to_end) {
    print_formatted_duration(time_to_end, { text_time_end }, false);
}

void PrintTime::print_timestamp_string_to_buffer(const time_t curr_sec, const uint32_t time_to_end, std::span<char> buffer) {
    static constexpr uint32_t FULL_DAY_SECONDS = 86400;
    time_t print_end_sec, tomorrow_sec;

    print_end_sec = curr_sec + time_to_end;
    tomorrow_sec = curr_sec + FULL_DAY_SECONDS;

    struct tm tomorrow, print_end, now;
    localtime_r(&curr_sec, &now);
    localtime_r(&tomorrow_sec, &tomorrow);
    localtime_r(&print_end_sec, &print_end);

    time_tools::TimeFormat time_format = time_tools::get_time_format();
    if (now.tm_mday == print_end.tm_mday && // if print end is today
        now.tm_mon == print_end.tm_mon && now.tm_year == print_end.tm_year) {
        FormatMsgPrintWillEnd::Today(buffer.data(), buffer.size(), &print_end, time_format == time_tools::TimeFormat::_24h);
    } else if (tomorrow.tm_mday == print_end.tm_mday && // if print end is tomorrow
        tomorrow.tm_mon == print_end.tm_mon && tomorrow.tm_year == print_end.tm_year) {
        FormatMsgPrintWillEnd::DayOfWeek(buffer.data(), buffer.size(), &print_end, time_format == time_tools::TimeFormat::_24h);
    } else {
        FormatMsgPrintWillEnd::Date(buffer.data(), buffer.size(), &print_end, time_format == time_tools::TimeFormat::_24h, FormatMsgPrintWillEnd::ISO);
    }
}

bool PrintTime::print_end_time(const uint32_t time_to_end, std::span<char> buffer) {
    time_t curr_sec = time(nullptr);
    if (curr_sec == -1) { // unable to fetch time
        return false;
    }

    curr_sec += time_tools::calculate_total_timezone_offset_minutes() * 60;

    print_timestamp_string_to_buffer(curr_sec, time_to_end, buffer);
    return true;
}

void PrintTime::generate_timestamp_string(const time_t curr_sec, const uint32_t time_to_end) {
    print_timestamp_string_to_buffer(curr_sec, time_to_end, { text_time_end });
}

Color PrintTime::generate_duration(time_t rawtime) {
    print_formatted_duration(static_cast<uint32_t>(rawtime), { text_time_dur }, true);
    // TODO: Print duration validation -> validation color
    return GuiDefaults::COLOR_VALUE_VALID;
}
