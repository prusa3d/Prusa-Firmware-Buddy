uint32_t time;

/* Try to ping domain example.com and print time */
if (esp_ping("example.com", &time, NULL, NULL, 1) == espOK) {
    printf("Ping successful. Time: %d ms\r\n", (int)time);
}