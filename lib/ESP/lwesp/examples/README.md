# ESP examples

Examples are split into different CPU architectures. Currently you can find examples for:

- `Visual Studio`: All examples will be always released for Visual studio, due to the fact that library is developed in it. You can run examples with NodeMCU hardware or any other hardware, if you connect it to USB and run with AT commands
- `STM32-Discovery`: Small amount of examples are available for STM32 based Discovery boards which come with socket for ESP-01 device. Examples are written in *Atollic TrueSTUDIO (GCC compiler)*, *Keil uVision (MDK-ARM compiler)* and *IAR*.

### ESP requirements

In order to run the examples, ESP device must run the AT software provided from Espressif systems.
Please check official documentation for detailed requirements to run this library with you ESP8266 device.

## WIN32 Examples

All windows based examples are written in Visual Studio 2017 as "Win32 project" and "Console Application".

### Visual Studio configuration

It may happen that Visual Studio sets different configuration on first project load and this may lead to wrong build and possible errors. Active configuration must be `Debug` and `Win32` or `x86`. Default active build can be set in project settings.

### NodeMCU development board

For development purposes, NodeMCU v3 board was used with virtual COM port support
to translate USB communication to UART required for ESP8266.

> Some NodeMCU boards have [CH340 USB->UART](https://www.tindie.com/products/multicognitive/nodemcu-esp8266-v3-lua-ch340-wifi-dev-board/) transceiver where I found problems with communication due to data loss between ESP and PC even at 115200 bauds. Try to find [NodeMCU with something else than CH340](https://www.ebay.com/itm/NodeMcu-Amica-V3-ESP-12E-ESP12E-4MB-FLASH-Lua-WIFI-Networking-dev-board-ESP8266-/141778019163).

### System functions for WIN32

Required system functions are based on "windows.h" file, available on windows operating system. Natively, there is support for:
1. Timing functions
2. Semaphores
3. Mutexes
4. Threads

The last part are message queues which are not implemented in Windows OS. Message queues were developed with help of semaphores and dynamic memory allocatations. System port for WIN32 is available in [esp_sys_win32.c](/src/system/esp_sys_win32.c) file.

### Communication with WIN32

Communication with NodeMCU hardware is using virtual files for COM ports. 
Implementation of low-level part (together with memory allocation for library) is available in [esp_ll_win32.c](/src/system/esp_ll_win32.c) file.

> In order to start using this port, user must set the appropriate COM port name when opening a virtual file. Please check implementation file for details.

## STM32F769I-Discovery

Use connector **CN2** to connect ESP-01 module with the board
```
Detailed pinout for STM32F769I-Discovery board

ESP-01 connection
- ESP_RX:           PC12
- ESP_TX:           PD2
- ESP_RESET:        PJ14

- UART:             UART5
- UART DMA:         DMA1
- UART DMA STREAM:  DMA_STREAM_0
- UART DMA CHANNEL: DMA_CHANNEL_4

DEBUG UART, connected through ST-LinkV2/1
- UART:             USART1
- UART_TX:          PA9
- UART_RX:          PA10
- UART baudrate:    921600
```

Driver implementation is available in [esp_ll_stm32f769i_discovery.c](/src/system/esp_ll_stm32f769i_discovery.c)

## STM32F723E-Discovery

Use connector **CN14** to connect ESP-01 module with the board
```
Detailed pinout for STM32F723E-Discovery board

ESP-01 connection
- ESP_RX:           PC12
- ESP_TX:           PD2
- ESP_RESET:        PG14
- ESP_CH_PD:        PD3
- ESP_GPIO_2:       PD6

- UART:             UART5
- UART DMA:         DMA1
- UART DMA STREAM:  DMA_STREAM_0
- UART DMA CHANNEL: DMA_CHANNEL_4

DEBUG UART, connected through ST-LinkV2/1
- UART:             USART6
- UART_TX:          PC6
- UART_RX:          PC7
- UART baudrate:    921600
```

Driver implementation is available in [esp_ll_stm32f723e_discovery.c](/src/system/esp_ll_stm32f723e_discovery.c) file.

## STM32L496G-Discovery

Discovery comes with STMOD+ extensions board which includes connector marked as **ESP-01**.

```
Detailed pinout for STM32L496G-Discovery board

ESP-01 connection
- ESP_RX:           PB6
- ESP_TX:           PG10, pin connected on VDDIO2 domain, make sure it is enabled in PWR registers
- ESP_RESET:        PB2
- ESP_CH_PD:        PA4
- ESP_GPIO_0:       PH0
- ESP_GPIO_2:       PA0

- UART:             USART1
- UART DMA:         DMA1
- UART DMA CHANNEL: DMA_CHANNEL_5
- UART DMA REQUEST: DMA_REQUEST_2

DEBUG UART, connected through ST-LinkV2/1
- UART:             USART2
- UART_TX:          PA2
- UART_RX:          PD6
- UART baudrate:    921600
```

Driver implementation is available in [esp_ll_stm32l496g_discovery.c](/src/system/esp_ll_stm32l496g_discovery.c)

## STM32L432KC-Nucleo

```
Detailed pinout for STM32L432KC-Nucleo board

ESP-01 connection
- ESP_RX:           PA9
- ESP_TX:           PA10
- ESP_RESET:        PA12
- ESP_CH_PD:        PB0
- ESP_GPIO_0:       PA7
- ESP_GPIO_2:       PA6

- UART:             USART1
- UART DMA:         DMA1
- UART DMA CHANNEL: DMA_CHANNEL_5
- UART DMA REQUEST: DMA_REQUEST_2

DEBUG UART, connected through ST-LinkV2/1
- UART:             USART2
- UART_TX:          PA2
- UART_RX:          PA3
- UART baudrate:    921600
```

Driver implementation is available in [esp_ll_stm32l432kc_nucleo.c](/src/system/esp_ll_stm32l432kc_nucleo.c)
