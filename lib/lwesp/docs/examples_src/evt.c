/*
 * \brief           User defined callback function for ESP events
 * \param[in]       evt: Callback event data
 */
lwespr_t
lwesp_callback_function(lwesp_evt_t* evt) {
    switch (lwesp_evt_get_type(evt)) {
        case LWESP_EVT_RESET: {                    /* Reset detected on ESP device */
            if (lwesp_evt_reset_is_forced(evt)) {  /* Check if forced by user */
                printf("Reset forced by user!\r\n");
            }
            break;
        }
        default: break;
    }
    return lwespOK;
}