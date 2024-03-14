all: slave master full

slave:
	avr-gcc -c avrtest.c -o slave.o -I../../include -Os -Wall -mmcu=atmega328 -DLIGHTMODBUS_SLAVE_FULL
	avr-size slave.o

master:
	avr-gcc -c avrtest.c -o master.o -I../../include -Os -Wall -mmcu=atmega328 -DLIGHTMODBUS_MASTER_FULL
	avr-size master.o

full:
	avr-gcc avrtest.c -o full.o -I../../include -Os -Wall -mmcu=atmega328 -DLIGHTMODBUS_FULL
	avr-size full.o
