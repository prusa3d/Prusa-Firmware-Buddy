/**
 * @file print_time_module.hpp
 * @author Michal Rudolf
 * @brief Module that acquires, holds and calculates time strings for printing screen (and others)
 * @date 2021-03-25
 */
#pragma once

#include "guitypes.h"
#include "i18n.h"
#include <array>
#include "window_text.hpp"
#include "marlin_server_shared.h"

enum class PT_t {
    init,
    countdown,
    timestamp,
};

class PrintTime {
public:
    static constexpr const char *EN_STR_COUNTDOWN = N_("Remaining time");
    static constexpr const char *EN_STR_TIMESTAMP = N_("Print will end");

    PT_t update_loop(PT_t screen_format, window_text_t *out_print_end, window_text_t *out_print_dur = nullptr);

private:
    static constexpr size_t MAX_END_TIMESTAMP_SIZE = 14 + 12 + 5; // "dd.mm.yyyy at hh:mm:ss" + safety measures for 3 digit where 2 digits should be
    static constexpr size_t MAX_TIMEDUR_STR_SIZE = 9;
    static constexpr uint32_t COUNTDOWN_TIME_S = 3 * 3600; // A time that is worth displaying countdown instead of a timestamp

    /**
     *  Print timestamp
     *
     *  It updates it's internal buffer and makeRAM the time string.
     *  Input values are consider to be already checked for invalidity.
     *
     *  @param [in] curr_sec - secounds from internal clock (fed by sntp)
     *  @param [in] time_to_end - seconds to end of the print
     *  @retval color_t - returns color based on time validation
     */
    void generate_timestamp_string(const time_t curr_sec, const uint32_t time_to_end);

    /**
     *  Print countdown
     *
     *  It updates it's internal buffer and makeRAM the time string. Prints countdown to end of the print.
     *  Input values are consider to be already checked for invalidity.
     *
     *  @param [in] time_to_end - seconds to end of the print
     *  @retval color_t - returns color based on time validation
     */
    void generate_countdown_string(const uint32_t time_to_end);

    /**
     *  Print duration
     *  It updates it's internal buffer and makeRAM the time string. Prints print duration.
     *
     *  @param [in] rawtime - seconds from epoch start, from internal clock (fed by sntp)
     *  @retval color_t - returns color based on time validation
     */
    color_t generate_duration(const time_t rawtime);

    inline static std::array<char, MAX_END_TIMESTAMP_SIZE> text_time_end; /**< Buffer for time to end (max 31 chars) */
    inline static std::array<char, MAX_TIMEDUR_STR_SIZE> text_time_dur;   /**< Buffer for time duration (max 9 chars) */

    PT_t time_end_format = PT_t::init;                  /**< Currently used time end format */
    uint32_t last_print_duration = TIME_TO_END_INVALID; /**< last recorded print_duration */
    uint32_t last_time_to_end = TIME_TO_END_INVALID;    /**< last end time used for GUI update */
};
