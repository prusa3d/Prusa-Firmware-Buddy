uint8_t initialized = 0;

/*
 * \brief           Send function callback which is called each time user must send something to AT port
 * \param[in]       data: Data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
static size_t
send_data(const void* data, size_t len) {
    return len;
}

/*
 * \brief           Core callback function which must be implemented by user
 * \param[in]       ll: Low-Level structure
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_ll_init(esp_ll_t* ll) {
#if !ESP_CFG_MEM_CUSTOM
    /*
     * In step 1, define memory array used for memory allocator
     * and send it to upper layer.
     *
     * Define memory regions where allocater may 
     * search for available memory.
     *
     * Since function may be called multiple times,
     * make sure you assign memory only first time function is called
     */
    static uint8_t memory[0x10000];
    esp_mem_region_t mem_regions[] = {
        { memory, sizeof(memory) }
    };
    if (!initialized) {
        esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions)); 
    }
#endif /* !ESP_CFG_MEM_CUSTOM */
    
    /*
     * Step 2 is to set the send callback function
     * which is called each time data have to be sent to AT port
     */
    if (!initialized) {
        ll->send_fn = send_data;
    }
    
    /*
     * In last step we have to configure AT port
     * to be able to receive and transmit data
     *
     * Since user may change baudrate in upper layer,
     * this function may be called multiple times
     */
    configure_uart(ll->uart.baudrate);
    
    initialized = 1;

    return espOK;
}