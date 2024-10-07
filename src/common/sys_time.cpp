#include <time.h>
#include "stm32f4xx_hal.h"
#include <buddy/main.h>

clock_t clock(void) {
    return HAL_GetTick() / CLOCKS_PER_SEC;
}

time_t time(time_t *timer) {

    time_t secs;

    RTC_TimeTypeDef curr_time;
    RTC_DateTypeDef curr_date;
    HAL_RTC_GetTime(&hrtc, &curr_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &curr_date, RTC_FORMAT_BIN);

    if (curr_date.Year == 0) { // Year 1900 -> RTC was not initialize (In 2036 it will overflow)
        if (timer) {
            *timer = -1;
        }
        return -1;
    }

    struct tm system_time;
    system_time.tm_isdst = -1; // Is DST on? 1 = yes, 0 = no, -1 = unknown
    system_time.tm_hour = curr_time.Hours;
    system_time.tm_min = curr_time.Minutes;
    system_time.tm_sec = curr_time.Seconds;
    system_time.tm_mday = curr_date.Date;
    system_time.tm_mon = curr_date.Month;
    system_time.tm_year = curr_date.Year;
    system_time.tm_wday = curr_date.WeekDay;
    secs = mktime(&system_time);

    if (timer) {
        *timer = secs;
    }
    return secs;
}
