MCU = atmega328p    # MCU type
F_CPU = 8000000UL   # Clock speed
BAUD_RATE = 9600    # USART0 baudrate
SLAVE_ADDRESS = 1   # Address of the slave
REG_COUNT = 32      # Number of registers
MAX_REQUEST = 64    # Max request length
MAX_RESPONSE = 64   # Max response length

CC = avr-gcc
DEFS = -DNDEBUG -DF_CPU=$(F_CPU) -DBAUD_RATE=$(BAUD_RATE) -DSLAVE_ADDRESS=$(SLAVE_ADDRESS) \
	-DREG_COUNT=$(REG_COUNT) -DMAX_RESPONSE=$(MAX_RESPONSE) -DMAX_REQUEST=$(MAX_REQUEST)
CFLAGS = -I../../include -Os -mmcu=$(MCU) -flto $(DEFS)

all: avrslave.elf makefile

avrslave.elf: avrslave.c makefile FORCE
	$(CC) $(CFLAGS) avrslave.c -o avrslave.elf
	avr-size avrslave.elf

prog:
	avrdude -c usbasp -p $(MCU) -U flash:w:avrslave.elf

FORCE: