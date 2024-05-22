# AVR Modbus RTU slave

This directory contains a fully-featured Modbus RTU slave implementation for
AVR microcontrollers (ATmega328p) utilizing around 3.7kB of the flash memory.

You can provide different configuration options to `make` in order to customize the build:
|Variable|Default|Description|
|--------|-------|-----------|
|`MCU`|`atmega328p`|MCU type|
|`F_CPU`|`8000000UL`|Clock speed|
|`BAUD_RATE`|`9600`|USART0 baudrate|
|`SLAVE_ADDRESS`|`1`|Address of the slave|
|`REG_COUNT`|`32`|Number of registers|
|`MAX_REQUEST`|`64`|Max request length|
|`MAX_RESPONSE`|`64`|Max response length|

This implementation uses USART0 for communication with the master. You can use the `linuxmaster` program from the `examples` directory to interact with the slave.