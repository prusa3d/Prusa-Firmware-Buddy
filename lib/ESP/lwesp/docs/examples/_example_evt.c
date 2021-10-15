/*
 * \brief           User defined callback function for ESP events
 * \param[in]       evt: Callback event data
 */
espr_t
esp_callback_function(esp_evt_t* evt) {
    switch (esp_evt_get_type(evt)) {
        case ESP_EVT_RESET: {                    /* Reset detected on ESP device */
            if (esp_evt_reset_is_forced(evt)) {  /* Check if forced by user */
                printf("Reset forced by user!\r\n");
            }
            break;
        }
        default: break;
    }
    return espOK;
}