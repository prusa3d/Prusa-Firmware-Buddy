size_t i, apf;
esp_ap_t aps[100];
 
/* Search for access points around ESP station */
if (esp_sta_list_ap(NULL, aps, ESP_ARRAYSIZE(aps), &apf, NULL, NULL, 1) == espOK) {
    for (i = 0; i < apf; i++) {
        printf("AP found: %s\r\n", aps[i].ssid);
    }
}