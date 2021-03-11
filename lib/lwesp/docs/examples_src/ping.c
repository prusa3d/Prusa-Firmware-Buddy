uint32_t time;

/* Try to ping domain example.com and print time */
if (lwesp_ping("example.com", &time, NULL, NULL, 1) == lwespOK) {
    printf("Ping successful. Time: %d ms\r\n", (int)time);
}