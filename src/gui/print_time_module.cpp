// print_time_module.cpp

#include "format_print_will_end.hpp"
#include <ctime>
#include "marlin_client.hpp"
#include "print_time_module.hpp"
#include "time_tools.hpp"
#include <config_store/store_instance.hpp>

PT_t PrintTime::update_loop(PT_t screen_format, window_text_t *out_print_end, [[maybe_unused]] window_text_t *out_print_dur) {
    // TODO: for MINI - Add time_dur condition
    // TODO: Non-context time <-> context time
    const uint32_t time_to_end = marlin_vars()->time_to_end;

    if (screen_format != PT_t::init && time_to_end != marlin_server::TIME_TO_END_INVALID && time_to_end == last_time_to_end) {
        return time_end_format;
    }

    const color_t print_end_text_color = [&] {
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
        const int8_t timezone_diff = config_store().timezone.get();
        const time_t local_cur_sec = curr_sec + timezone_diff * 3600;
        time_end_format = PT_t::timestamp;
        generate_timestamp_string(local_cur_sec, time_to_end);

        // Add unknown marker
        if (marlin_vars()->print_speed != 100) {
            strlcat(text_time_end.data(), "?", MAX_END_TIMESTAMP_SIZE);
        }

        return GuiDefaults::COLOR_VALUE_VALID;
    }();
    out_print_end->SetTextColor(print_end_text_color);
    out_print_end->SetText(string_view_utf8::MakeRAM((const uint8_t *)text_time_end.data()));
    out_print_end->Invalidate();
    last_time_to_end = time_to_end;

    if (out_print_dur) {
        const time_t rawtime = (time_t)marlin_vars()->print_duration;
        if (rawtime != last_print_duration) {
            out_print_dur->SetTextColor(generate_duration(rawtime));
            out_print_dur->SetText(string_view_utf8::MakeRAM((const uint8_t *)text_time_dur.data()));
        }
    }

    return time_end_format;
}

void PrintTime::generate_countdown_string(const uint32_t time_to_end) {
    time_t rawtime = time_t(time_to_end);
    const struct tm *timeinfo = localtime(&rawtime);
    // standard would be:
    // strftime(array.data(), array.size(), "%jd %Hh", timeinfo);
    if (timeinfo->tm_yday) {
        snprintf(text_time_end.data(), MAX_END_TIMESTAMP_SIZE, "%id %2ih", timeinfo->tm_yday, timeinfo->tm_hour);
    } else if (timeinfo->tm_hour) {
        snprintf(text_time_end.data(), MAX_END_TIMESTAMP_SIZE, "%ih %2im", timeinfo->tm_hour, timeinfo->tm_min);
    } else {
        snprintf(text_time_end.data(), MAX_END_TIMESTAMP_SIZE, "%im", timeinfo->tm_min);
    }
}

void PrintTime::generate_timestamp_string(const time_t curr_sec, const uint32_t time_to_end) {
    static constexpr uint32_t FULL_DAY_SECONDS = 86400;
    time_t print_end_sec, tomorrow_sec;

    print_end_sec = curr_sec + time_to_end;
    tomorrow_sec = curr_sec + FULL_DAY_SECONDS;

    struct tm tomorrow, print_end, now;
    localtime_r(&curr_sec, &now);
    localtime_r(&tomorrow_sec, &tomorrow);
    localtime_r(&print_end_sec, &print_end);

    time_format::TF_t time_format = time_format::Get();
    if (now.tm_mday == print_end.tm_mday && // if print end is today
        now.tm_mon == print_end.tm_mon && now.tm_year == print_end.tm_year) {
        FormatMsgPrintWillEnd::Today(text_time_end.data(), MAX_END_TIMESTAMP_SIZE, &print_end, time_format == time_format::TF_t::TF_24H);
    } else if (tomorrow.tm_mday == print_end.tm_mday && // if print end is tomorrow
        tomorrow.tm_mon == print_end.tm_mon && tomorrow.tm_year == print_end.tm_year) {
        FormatMsgPrintWillEnd::DayOfWeek(text_time_end.data(), MAX_END_TIMESTAMP_SIZE, &print_end, time_format == time_format::TF_t::TF_24H);
    } else {
        FormatMsgPrintWillEnd::Date(text_time_end.data(), MAX_END_TIMESTAMP_SIZE, &print_end, time_format == time_format::TF_t::TF_24H, FormatMsgPrintWillEnd::ISO);
    }
}

color_t PrintTime::generate_duration(time_t rawtime) {
    const struct tm *timeinfo = localtime(&rawtime);
    if (timeinfo->tm_yday) {
        snprintf(text_time_dur.data(), MAX_TIMEDUR_STR_SIZE, "%id %2ih", timeinfo->tm_yday, timeinfo->tm_hour);
    } else if (timeinfo->tm_hour) {
        snprintf(text_time_dur.data(), MAX_TIMEDUR_STR_SIZE, "%ih %2im", timeinfo->tm_hour, timeinfo->tm_min);
    } else if (timeinfo->tm_min) {
        snprintf(text_time_dur.data(), MAX_TIMEDUR_STR_SIZE, "%im %2is", timeinfo->tm_min, timeinfo->tm_sec);
    } else {
        snprintf(text_time_dur.data(), MAX_TIMEDUR_STR_SIZE, "%is", timeinfo->tm_sec);
    }
    // TODO: Print duration validation -> validation color
    return GuiDefaults::COLOR_VALUE_VALID;
}
