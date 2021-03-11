size_t i, apf;
lwesp_ap_t aps[100];

/* Search for access points around ESP station */
if (lwesp_sta_list_ap(NULL, aps, LWESP_ARRAYSIZE(aps), &apf, NULL, NULL, 1) == lwespOK) {
    for (i = 0; i < apf; i++) {
        printf("AP found: %s\r\n", aps[i].ssid);
    }
}