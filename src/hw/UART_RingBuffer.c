#include "UART_RingBuffer.h"
#include <string.h>

extern UART_HandleTypeDef huart6;

ring_buffer rx_buffer1 = { { 0 }, 0, 0 };
ring_buffer tx_buffer1 = { { 0 }, 0, 0 };

ring_buffer *_rx_buffer1;
ring_buffer *_tx_buffer1;

void store_char(unsigned char c, ring_buffer *buffer);

void Ringbuf_init(void) {
    _rx_buffer1 = &rx_buffer1;
    _tx_buffer1 = &tx_buffer1;

    /* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_ERR);
    /* Enable the UART Data Register not empty Interrupt */
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_RXNE);
}

void store_char(unsigned char c, ring_buffer *buffer) {
    unsigned int i = (unsigned int)(buffer->head + 1) % UART_BUFFER_SIZE;
    // if we should be storing the received character into the location
    // just before the tail (meaning that the head would advance to the
    // current location of the tail), we're about to overflow the buffer
    // and so we don't write the character or advance the head.
    if (i != buffer->tail) {
        buffer->buffer[buffer->head] = c;
        buffer->head = i;
    }
}

int Look_for(char *str, char *buffertolookinto) {
    int stringlength = strlen(str);
    int bufferlength = strlen(buffertolookinto);
    int so_far = 0;
    int indx = 0;
repeat:
    while (str[so_far] != buffertolookinto[indx])
        indx++;
    if (str[so_far] == buffertolookinto[indx]) {
        while (str[so_far] == buffertolookinto[indx]) {
            so_far++;
            indx++;
        }
    }

    else {
        so_far = 0;
        if (indx >= bufferlength)
            return -1;
        goto repeat;
    }

    if (so_far == stringlength)
        return 1;
    else
        return -1;
}

void GetDataFromBuffer(char *startString, char *endString, char *buffertocopyfrom, char *buffertocopyinto) {
    int startStringLength = strlen(startString);
    int endStringLength = strlen(endString);
    int so_far = 0;
    int indx = 0;
    int startposition = 0;
    int endposition = 0;

repeat1:
    while (startString[so_far] != buffertocopyfrom[indx])
        indx++;
    if (startString[so_far] == buffertocopyfrom[indx]) {
        while (startString[so_far] == buffertocopyfrom[indx]) {
            so_far++;
            indx++;
        }
    }

    if (so_far == startStringLength)
        startposition = indx;
    else {
        so_far = 0;
        goto repeat1;
    }

    so_far = 0;

repeat2:
    while (endString[so_far] != buffertocopyfrom[indx])
        indx++;
    if (endString[so_far] == buffertocopyfrom[indx]) {
        while (endString[so_far] == buffertocopyfrom[indx]) {
            so_far++;
            indx++;
        }
    }

    if (so_far == endStringLength)
        endposition = indx - endStringLength;
    else {
        so_far = 0;
        goto repeat2;
    }

    so_far = 0;
    indx = 0;

    for (int i = startposition; i < endposition; i++) {
        buffertocopyinto[indx] = buffertocopyfrom[i];
        indx++;
    }
}

void Uart_flush(UART_HandleTypeDef *uart) {
    memset(_rx_buffer1->buffer, '\0', UART_BUFFER_SIZE);
    _rx_buffer1->head = 0;
}

int Uart_peek(UART_HandleTypeDef *uart) {
    if (_rx_buffer1->head == _rx_buffer1->tail) {
        return -1;
    } else {
        return _rx_buffer1->buffer[_rx_buffer1->tail];
    }
    return -1;
}

int Uart_read(UART_HandleTypeDef *uart) {
    // if the head isn't ahead of the tail, we don't have any characters
    if (_rx_buffer1->head == _rx_buffer1->tail) {
        return -1;
    } else {
        unsigned char c = _rx_buffer1->buffer[_rx_buffer1->tail];
        _rx_buffer1->tail = (unsigned int)(_rx_buffer1->tail + 1) % UART_BUFFER_SIZE;
        return c;
    }
    return -1;
}

void Uart_write(int c, UART_HandleTypeDef *uart) {
    if (c >= 0) {
        unsigned int i = (_tx_buffer1->head + 1) % UART_BUFFER_SIZE;

        // If the output buffer is full, there's nothing for it other than to
        // wait for the interrupt handler to empty it a bit
        // ???: return 0 here instead?
        while (i == _tx_buffer1->tail)
            ;

        _tx_buffer1->buffer[_tx_buffer1->head] = (uint8_t)c;
        _tx_buffer1->head = i;

        __HAL_UART_ENABLE_IT(&huart6, UART_IT_TXE); // Enable UART transmission interrupt
    }
}

int IsDataAvailable(UART_HandleTypeDef *uart) {
    return (uint16_t)(UART_BUFFER_SIZE + _rx_buffer1->head - _rx_buffer1->tail) % UART_BUFFER_SIZE;
}

int Get_after(char *string, uint8_t numberofchars, char *buffertosave, UART_HandleTypeDef *uart) {

    while (Wait_for(string, uart) != 1)
        ;
    for (int indx = 0; indx < numberofchars; indx++) {
        while (!(IsDataAvailable(uart)))
            ;
        buffertosave[indx] = Uart_read(uart);
    }
    return 1;
}

void Uart_sendstring(const char *s, UART_HandleTypeDef *uart) {
    while (*s != '\0')
        Uart_write(*s++, uart);
}

void Uart_printbase(long n, uint8_t base, UART_HandleTypeDef *uart) {
    char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
    char *s = &buf[sizeof(buf) - 1];

    *s = '\0';

    // prevent crash if called with base == 1
    if (base < 2)
        base = 10;

    do {
        unsigned long m = n;
        n /= base;
        char c = m - base * n;
        *--s = c < 10 ? c + '0' : c + 'A' - 10;
    } while (n);

    while (*s)
        Uart_write(*s++, uart);
}

int Copy_upto(char *string, char *buffertocopyinto, UART_HandleTypeDef *uart) {
    int so_far = 0;
    int len = strlen(string);
    int indx = 0;

again:
    while (!IsDataAvailable(uart))
        ;
    while (Uart_peek(uart) != string[so_far]) {
        buffertocopyinto[indx] = _rx_buffer1->buffer[_rx_buffer1->tail];
        _rx_buffer1->tail = (unsigned int)(_rx_buffer1->tail + 1) % UART_BUFFER_SIZE;
        indx++;
        while (!IsDataAvailable(uart))
            ;
    }
    while (Uart_peek(uart) == string[so_far]) {
        so_far++;
        buffertocopyinto[indx++] = Uart_read(uart);
        if (so_far == len)
            return 1;
        while (!IsDataAvailable(uart))
            ;
    }

    if (so_far != len) {
        so_far = 0;
        goto again;
    }

    if (so_far == len)
        return 1;
    else
        return -1;
}

int Wait_for(char *string, UART_HandleTypeDef *uart) {
    int so_far = 0;
    int len = strlen(string);

again_device:
    while (!IsDataAvailable(uart))
        ;
    if (Uart_peek(uart) != string[so_far]) {
        _rx_buffer1->tail = (unsigned int)(_rx_buffer1->tail + 1) % UART_BUFFER_SIZE;
        goto again_device;
    }
    while (Uart_peek(uart) == string[so_far]) {
        so_far++;
        Uart_read(uart);
        if (so_far == len)
            return 1;
        while (!IsDataAvailable(uart))
            ;
    }

    if (so_far != len) {
        so_far = 0;
        goto again_device;
    }

    if (so_far == len)
        return 1;
    else
        return -1;
}

void Uart_isr(UART_HandleTypeDef *huart) {
    uint32_t isrflags = READ_REG(huart->Instance->SR);
    uint32_t cr1its = READ_REG(huart->Instance->CR1);

    /* if DR is not empty and the Rx Int is enabled */
    if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET)) {
        /******************
    	    	      *  @note   PE (Parity error), FE (Framing error), NE (Noise error), ORE (Overrun
    	    	      *          error) and IDLE (Idle line detected) flags are cleared by software
    	    	      *          sequence: a read operation to USART_SR register followed by a read
    	    	      *          operation to USART_DR register.
    	    	      * @note   RXNE flag can be also cleared by a read to the USART_DR register.
    	    	      * @note   TC flag can be also cleared by software sequence: a read operation to
    	    	      *          USART_SR register followed by a write operation to USART_DR register.
    	    	      * @note   TXE flag is cleared only by a write to the USART_DR register.

    	 *********************/
        huart->Instance->SR;                   /* Read status register */
        unsigned char c = huart->Instance->DR; /* Read data register */
        store_char(c, _rx_buffer1);            // store data in buffer

        return;
    }

    /*If interrupt is caused due to Transmit Data Register Empty */
    if (((isrflags & USART_SR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET)) {
        if (tx_buffer1.head == tx_buffer1.tail) {
            // Buffer empty, so disable interrupts
            __HAL_UART_DISABLE_IT(huart, UART_IT_TXE);
        } else {
            // There is more data in the output buffer. Send the next byte
            unsigned char c = tx_buffer1.buffer[tx_buffer1.tail];
            tx_buffer1.tail = (tx_buffer1.tail + 1) % UART_BUFFER_SIZE;

            /******************
    	      *  @note   PE (Parity error), FE (Framing error), NE (Noise error), ORE (Overrun
    	      *          error) and IDLE (Idle line detected) flags are cleared by software
    	      *          sequence: a read operation to USART_SR register followed by a read
    	      *          operation to USART_DR register.
    	      * @note   RXNE flag can be also cleared by a read to the USART_DR register.
    	      * @note   TC flag can be also cleared by software sequence: a read operation to
    	      *          USART_SR register followed by a write operation to USART_DR register.
    	      * @note   TXE flag is cleared only by a write to the USART_DR register.

    	      *********************/

            huart->Instance->SR;
            huart->Instance->DR = c;
        }
        return;
    }
}
