lwesp_datetime_t dt;

/* Configure SNTP parameters: enable, timezone +1 and use default network servers */
if (lwesp_sntp_set_config(1, 1, NULL, NULL, NULL, NULL, NULL, 1) == lwespOK) {
    /* Try to get time from network servers */
    if (lwesp_sntp_gettime(&dt, NULL, NULL, 1) == lwespOK) {
        printf("We have a date and time: %d.%d.%d: %d:%d:%d\r\n",
            (int)dt.date, (int)dt.month, (int)dt.year,
            (int)dt.hours, (int)dt.minutes, (int)dt.seconds
        );
    }
}