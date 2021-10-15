esp_datetime_t dt;

/* Configure SNTP parameters: enable, timezone +1 and use default network servers */
if (esp_sntp_configure(1, 1, NULL, NULL, NULL, NULL, NULL, 1) == espOK) {
    /* Try to get time from network servers */
    if (esp_sntp_gettime(&dt, NULL, NULL, 1) == espOK) {
        printf("We have a date and time: %d.%d.%d: %d:%d:%d\r\n", 
            (int)dt.date, (int)dt.month, (int)dt.year, 
            (int)dt.hours, (int)dt.minutes, (int)dt.seconds
        );
    }
}