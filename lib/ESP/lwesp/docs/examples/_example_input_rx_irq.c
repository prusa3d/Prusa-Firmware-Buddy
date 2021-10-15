/*
 * esp_ll.c file with LL communication
 * enables RX interrupt for AT port communication
 *
 *
 */
...
esp_ll_init(...) {
	...
	...
	...

	/* 
	 * Configure UART for communication
	 * and enable AT RX received data interrupt
	 */
	configure_uart(...);
}

/*
 * IRQ function for received data on AT port
 */
void
AT_PORT_IRQ_Handler(void) {
	uint8_t ch;

	/* Read received character from AT port */
	ch = ...

	/*
	 * Send character to upper buffer layer
	 * 
	 * Use esp_input function which will write element
	 * to input buffer with raw data
	 */
	esp_input(&ch, 1);
}