#include <stm32f4xx_hal.h>
#include <wui_api.h>

#include <time.h>

extern RTC_HandleTypeDef hrtc;
// In wui_api.cpp
extern bool sntp_time_init;

time_t time(time_t *timer) {
    if (sntp_time_init) {
        RTC_TimeTypeDef currTime;
        RTC_DateTypeDef currDate;
        HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &currDate, RTC_FORMAT_BIN);
        time_t secs;
        struct tm system_time;
        system_time.tm_isdst = -1; // Is DST on? 1 = yes, 0 = no, -1 = unknown
        system_time.tm_hour = currTime.Hours;
        system_time.tm_min = currTime.Minutes;
        system_time.tm_sec = currTime.Seconds;
        system_time.tm_mday = currDate.Date;
        system_time.tm_mon = currDate.Month;
        system_time.tm_year = currDate.Year;
        system_time.tm_wday = currDate.WeekDay;
        secs = mktime(&system_time);
        if (timer != NULL) {
            *timer = secs;
        }
        return secs;
    } else {
        if (timer != NULL) {
            *timer = -1;
        }
        return -1;
    }
}
