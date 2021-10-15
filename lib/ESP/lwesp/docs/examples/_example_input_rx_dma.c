/*
 * esp_ll.c file with LL communication
 * creates a new thread for reading data received on AT port
 * and enables DMA to send received data to internal buffer
 */
...
esp_ll_init(...) {
    ...
    ...
    ...

    /* 
     * Configure UART for communication
     * and enable DMA for RX and create a new thread
     */
    configure_uart(...);
}


/*
 * Create buffer to receive data to
 */
uint8_t rx_buff[0x100];
uint16_t last_pos;

/*
 * RX data thread function
 * which will check the data every 1ms and will send it to upper layer
 */
void
rx_dma_thread(void) {
    while (1) {
        /* Get the current DMA write pointer */
        /* Calculate pointer from last reading and get the difference for buffer reading */
        
        /* When buffer memory is known, call a processing function */
        esp_input_process(buffer, len);
        
        /* Do a little delay to allow other threads to process */
        osDelay(1);
    }
}