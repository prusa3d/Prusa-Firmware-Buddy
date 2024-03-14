#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef DEBUG
#define LIGHTMODBUS_DEBUG
#endif

#define LIGHTMODBUS_SLAVE_FULL
#define LIGHTMODBUS_IMPL
#include <lightmodbus/lightmodbus.h>

#ifndef F_CPU
#error F_CPU not defined!
#endif

#ifndef BAUD_RATE
#define BAUD_RATE 9600
#endif

#ifndef REG_COUNT
#define REG_COUNT 16
#endif

#ifndef MAX_REQUEST
#define MAX_REQUEST 64
#endif

#ifndef MAX_RESPONSE
#define MAX_RESPONSE 64
#endif

#ifndef SLAVE_ADDRESS
#define SLAVE_ADDRESS 1
#endif

#ifndef RS485_DIR_RX
#define RS485_DIR_RX
#endif

#ifndef RS485_DIR_TX
#define RS485_DIR_TX
#endif

#ifdef DEBUG
#define DEBUG_ONLY(x) x
#else
#define DEBUG_ONLY(x)
#endif

#define RX_INT_ENABLE  (UCSR0B |= (1 << RXCIE0))
#define RX_INT_DISABLE (UCSR0B &= ~(1 << RXCIE0))

/*
	Returns a value that should be assigned to OCR0A in order to wait
	n/10 character times (assuming x1024 prescaler, BAUD_RATE and F_CPU)
*/
#define TIMEOUT_T(n) ((uint8_t)((9ull * F_CPU * (n) / 10) / (1024ull * BAUD_RATE)))
#define TIMEOUT_T20 TIMEOUT_T(20)
#define TIMEOUT_T15 TIMEOUT_T(15)

/*
	States of the slave state machine
*/
typedef enum SlaveState
{
	STATE_IDLE,
	STATE_RECEIVING,
	STATE_WAITING,
	STATE_WAITING_INVALID,
} SlaveState;

/*
	Request data. Written inside an interrupt
	hence the 'volatile'.
*/
static volatile struct
{
	uint8_t data[MAX_REQUEST]; //!< Received data
	uint8_t charReceivedFlag;  //!< Set whenever a new char is appended
	uint16_t length;           //!< Length of the request
} rx;

/*
	USART0 receive interrupt
*/
ISR(USART_RX_vect)
{
	uint8_t c = UDR0;
	rx.charReceivedFlag = 1;
	if (rx.length < MAX_REQUEST)
		rx.data[rx.length++] = c;
}

/*
	Timeout flag
*/
static volatile uint8_t timeoutFlag = 0;

/*
	Arrays storing register and coil values.
*/
uint16_t regs[REG_COUNT];
uint8_t coils[REG_COUNT / 8];

/*
	The timeout interrupt
*/
ISR(TIMER0_COMPA_vect)
{
	timeoutFlag = 1;
	TCCR0B = 0; // Stop timer
}

/*
	A custom allocator. Returns memory
	from a statically allocated array.
*/
ModbusError staticAllocator(
	ModbusBuffer *buffer,
	uint16_t size,
	void *context)
{
	// Array for holding the response frame
	static uint8_t response[MAX_RESPONSE];

	if (size != 0) // Allocation reqest
	{
		if (size <= MAX_RESPONSE) // Allocation request is within bounds
		{
			buffer->data = response;
			return MODBUS_OK;
		}
		else // Allocation error
		{
			buffer->data = NULL;
			return MODBUS_ERROR_ALLOC;
		}
	}
	else // Free request
	{
		buffer->data = NULL;
		return MODBUS_OK;
	}
}

/*
	A simple register callback
*/
ModbusError regCallback(
	const ModbusSlave *slave,
	const ModbusRegisterCallbackArgs *args,
	ModbusRegisterCallbackResult *result)
{
	switch (args->query)
	{
		// All regs can be read
		case MODBUS_REGQ_R_CHECK:
			if (args->index < REG_COUNT)
				result->exceptionCode = MODBUS_EXCEP_NONE;
			else	
				result->exceptionCode = MODBUS_EXCEP_ILLEGAL_ADDRESS;
			break;
			
		// All but two last regs/coils can be written
		case MODBUS_REGQ_W_CHECK:
			if (args->index < REG_COUNT - 2)
				result->exceptionCode = MODBUS_EXCEP_NONE;
			else	
				result->exceptionCode = MODBUS_EXCEP_SLAVE_FAILURE;
			break;

		// Read registers
		case MODBUS_REGQ_R:
			switch (args->type)
			{
				case MODBUS_HOLDING_REGISTER: result->value = regs[args->index]; break;
				case MODBUS_INPUT_REGISTER: result->value = regs[args->index]; break;
				case MODBUS_COIL: result->value = modbusMaskRead(coils, args->index); break;
				case MODBUS_DISCRETE_INPUT: result->value = modbusMaskRead(coils, args->index); break;
			}
			break;

		// Write registers
		case MODBUS_REGQ_W:
			switch (args->type)
			{
				case MODBUS_HOLDING_REGISTER: regs[args->index] = args->value; break;
				case MODBUS_COIL: modbusMaskWrite(coils, args->index, args->value); break;
				default: abort(); break;
			}
			break;
	}

	return MODBUS_OK;
}

/*
	Transmits a single byte over USART
*/
static inline void usartTXB(uint8_t b)
{
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = b;
}

/*
	Transmits bytes over USART (RS485)
*/
static void usartTX(const uint8_t *data, uint16_t length)
{
	// This is because we want to avoid
	// turning on the driver even if the for loop
	// does 0 iterations
	if (!length) return;

	// Turn on RS485 driver
	RS485_DIR_TX;

	for (uint16_t i = 0; i < length; i++)
		usartTXB(data[i]);

	// Turn off RS485 driver
	RS485_DIR_RX;
}

/*
	Transmit string over USART0. Used only for debug
*/
static void usartTXS(const char *s)
{
	while (*s)
		usartTXB(*s++);
}

/*
	printf() for USART0
*/
 __attribute__((format(printf, 1, 2))) int uprintf(const char *fmt, ...)
{
	static char buf[64];
	va_list args;
	va_start(args, fmt);
	int result = vsnprintf(buf, sizeof buf, fmt, args);
	va_end(args);
	usartTXS(buf);
	return result;
}

/*
	Calls abort(), but prints out a fancy message beforehand
*/
static void die(const char *s)
{
	DEBUG_ONLY(uprintf("\n\r%s\n\r", s));
	abort();
}

/*
	Returns 1 if a byte has been received
*/
static inline uint8_t usartCheckReceived()
{
	return rx.charReceivedFlag;
}

/*
	Clears the 'char received' flag
*/
static inline void usartClearReceieved()
{
	rx.charReceivedFlag = 0;
}

/*
	Initializes timer 0 to generate an interrupt
	and set the timeout flag after a given time
*/
static inline void timeoutInit(uint8_t timeout)
{
	TCCR0A = (1 << WGM01); // CTC mode
	TCCR0B = (1 << CS02) | (1 << CS00); // Prescaler 1024
	TIMSK0 = (1 << OCIE0A);
	OCR0A = 255;//timeout;
	timeoutFlag = 0;
}

/*
	Re-starts the current timeout
*/
static inline void timeoutReset()
{
	TCNT0 = 0;
	timeoutFlag = 0;
}

/*
	Aborts the current timeout
*/
static inline void timeoutAbort()
{
	TCCR0B = 0;
	timeoutFlag = 0;
}

/*
	Returns 1 if the timeout has expired
*/
static inline uint8_t timeoutCheck()
{
	return timeoutFlag;
}

/*
	Process request and send response to the master
*/
void handleRequest(ModbusSlave *slave, const uint8_t *data, uint16_t length)
{
	DEBUG_ONLY(uprintf("Got %d bytes\n\r", length));

	// Attempt to parse the received frame
	ModbusErrorInfo err = modbusParseRequestRTU(
		slave,
		SLAVE_ADDRESS,
		data,
		length
	);

	// We ignore request/response errors 
	// and only care about the serious stuff
	switch (modbusGetGeneralError(err))
	{
		// We're fine
		case MODBUS_OK:
			break;

		// Since we're only doing static memory allocation
		// we can nicely handle memory allocation errors
		// and respond with a slave failure exception
		case MODBUS_ERROR_ALLOC:
			
			// We must be able to retrieve the function code byte
			if (length < 2)
				break;

			err = modbusBuildExceptionRTU(
				slave,
				SLAVE_ADDRESS,
				data[1],
				MODBUS_EXCEP_SLAVE_FAILURE);
			
			// Error while handling error. We die.
			if (!modbusIsOk(err))
				die("Unable to recover from MODBUS_ERROR_ALLOC");

			break;

		// Oh no.
		default:
			DEBUG_ONLY(usartTXS(modbusErrorStr(modbusGetGeneralError(err))));
			die("Uncaught general error");
			break;
	}

	// Respond only if the response can be accessed
	// and has non-zero length
	if (modbusIsOk(err) && modbusSlaveGetResponseLength(slave))
		usartTX(
			modbusSlaveGetResponse(slave),
			modbusSlaveGetResponseLength(slave));
}

int main(void)
{
	// USART0 init
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
	UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
	UBRR0 = (uint16_t) (F_CPU / 16 / BAUD_RATE - 1);

	// Init slave instance
	ModbusErrorInfo err;
	ModbusSlave slave;
	err = modbusSlaveInit(
		&slave,
		regCallback,
		NULL,
		staticAllocator,
		modbusSlaveDefaultFunctions,
		modbusSlaveDefaultFunctionCount);
	assert(modbusIsOk(err));

	// Enable interrupts and start receiving
	SlaveState state = STATE_IDLE;
	sei();

	while (1)
	{
		switch (state)
		{
			// Waiting for a char
			case STATE_IDLE:
				// Ensure we don't accidentally erase the first byte
				// Never uprintf() anything from here, because it takes too long
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
				{
					if (usartCheckReceived())
					{
						state = STATE_RECEIVING;
						usartClearReceieved();
						timeoutInit(TIMEOUT_T15);
					}
					else
					{
						rx.length = 0;
					}
				}
				break;

			// Receiving loop
			case STATE_RECEIVING:
				if (usartCheckReceived())
				{
					usartClearReceieved();
					timeoutReset();
				}
				else
				{
					// If we didn't receive any char in the last 1.5t,
					// go to waiting state, and expect additional 2.0t
					// of silence
					if (timeoutCheck())
					{
						state = STATE_WAITING;
						DEBUG_ONLY(usartTXS("receiving -> waiting\n"));
						timeoutInit(TIMEOUT_T20);
					}
				}
				break;

			// Wait additional 2.0t
			case STATE_WAITING:
				// If a char is received, mark the frame as invalid
				if (usartCheckReceived())
				{
					DEBUG_ONLY(usartTXS("waiting -> invalid\n"));
					state = STATE_WAITING_INVALID;
				}

				// Attempt to parse the frame
				{
					uint8_t ready = 0;

					// If we didn't receive any char in the last 2.0t,
					// get ready to parse the frame. We must ensure that
					// the frame is not modified while we're parsing it, so
					// we disable RX interrupts here. Technically, we could 
					// just parse the frame inside this atomic block here,
					// but that would be stupid, so we're not doing it.
					ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
					{
						if (!usartCheckReceived() && timeoutCheck())
						{
							RX_INT_DISABLE;
							ready = 1;	
						}
					}

					// If the frame is ready to parse, do it
					if (ready)
					{
						// Parse & prepare next state
						DEBUG_ONLY(usartTXS("parsing -> idle\n"));
						handleRequest(
							&slave,
							(const uint8_t*) rx.data, // Cast away the volatile (RX int is disabled)
							rx.length);
						state = STATE_IDLE;
						RX_INT_ENABLE;
					}
				}
				break;

			// Go back to the idle state without parsing the frame
			// after the timeout fires
			case STATE_WAITING_INVALID:
				if (timeoutCheck())
				{
					DEBUG_ONLY(usartTXS("invalid -> idle\n"));
					state = STATE_IDLE;
				}
				break;
		}
	}

	// Technically, we should destroy ModbusSlave here,
	// but... well...
	// modbusSlaveDestroy(&slave);
}