/*
 * UartRingbuffer.h
 *
 *  Created on: 10-Jul-2019
 *      Author: Controllerstech
 */

#ifndef UARTRINGBUFFER_H_
#define UARTRINGBUFFER_H_


#include "stm32f4xx_hal.h"

/* change the size of the buffer */
#define UART_BUFFER_SIZE 512

typedef struct
{
  unsigned char buffer[UART_BUFFER_SIZE];
  volatile unsigned int head;
  volatile unsigned int tail;
} ring_buffer;


/* reads the data in the rx_buffer and increment the tail count in rx_buffer of the given UART */
int Uart_read(UART_HandleTypeDef *uart);

/* writes the data to the tx_buffer and increment the head count in tx_buffer */
void Uart_write(int c, UART_HandleTypeDef *uart);

/* function to send the string to the uart */
void Uart_sendstring(const char *s, UART_HandleTypeDef *uart);

/* Print a number with any base
 * base can be 10, 8 etc*/
void Uart_printbase (long n, uint8_t base, UART_HandleTypeDef *uart);

/* Initialize the ring buffer */
void Ringbuf_init(void);

/* Resets the entire ring buffer, the new data will start from position 0 */
void Uart_flush (UART_HandleTypeDef *uart);

/* checks if the data is available to read in the rx_buffer of the uart */
int IsDataAvailable(UART_HandleTypeDef *uart);

/* Look for a particular string in the given buffer
 * @return 1, if the string is found and -1 if not found
 * @USAGE:: if (Look_for ("some string", buffer)) do something
 */
int Look_for (char *str, char *buffertolookinto);

/* Copies the required data from a buffer
 * @startString: the string after which the data need to be copied
 * @endString: the string before which the data need to be copied
 * @USAGE:: GetDataFromBuffer ("name=", "&", buffertocopyfrom, buffertocopyinto);
 */
void GetDataFromBuffer (char *startString, char *endString, char *buffertocopyfrom, char *buffertocopyinto);

/* Peek for the data in the Rx Bffer without incrementing the tail count
* Returns the character
* USAGE: if (Uart_peek () == 'M') do something
*/
int Uart_peek(UART_HandleTypeDef *uart);


/* Copy the data from the Rx buffer into the buffer, Upto and including the entered string
* This copying will take place in the blocking mode, so you won't be able to perform any other operations
* Returns 1 on success and -1 otherwise
* USAGE: while (!(Copy_Upto ("some string", buffer, uart)));
*/
int Copy_upto (char *string, char *buffertocopyinto, UART_HandleTypeDef *uart);


/* Copies the entered number of characters (blocking mode) from the Rx buffer into the buffer, after some particular string is detected
* Returns 1 on success and -1 otherwise
* USAGE: while (!(Get_after ("some string", 6, buffer, uart)));
*/
int Get_after (char *string, uint8_t numberofchars, char *buffertosave, UART_HandleTypeDef *uart);

/* Wait until a paricular string is detected in the Rx Buffer
* Return 1 on success and -1 otherwise
* USAGE: while (!(Wait_for("some string", uart)));
*/
int Wait_for (char *string, UART_HandleTypeDef *uart);

/* the ISR for the uart. put it in the IRQ handler */
void Uart_isr (UART_HandleTypeDef *huart);

/*** Depreciated For now. This is not needed, try using other functions to meet the requirement ***/
/* get the position of the given string within the incoming data.
 * It returns the position, where the string ends
 */
/* get the position of the given string in the given UART's incoming data.
 * It returns the position, where the string ends */
//int16_t Get_position (char *string, UART_HandleTypeDef *uart);


#endif /* UARTRINGBUFFER_H_ */
