.. _examples:

Examples and demos
==================

Various examples are provided for fast library evaluation on embedded systems. These are prepared and maintained for ``2`` platforms, but could be easily extended to more platforms:

* WIN32 examples, prepared as `Visual Studio Community <https://visualstudio.microsoft.com/vs/community/>`_ projects
* ARM Cortex-M examples for STM32, prepared as `STM32CubeIDE <https://www.st.com/en/development-tools/stm32cubeide.html>`_ GCC projects

.. warning::
	Library is platform independent and can be used on any platform.

Example architectures
^^^^^^^^^^^^^^^^^^^^^

There are many platforms available today on a market, however supporting them all would be tough task for single person.
Therefore it has been decided to support (for purpose of examples) ``2`` platforms only, `WIN32` and `STM32`.

WIN32
*****

Examples for *WIN32* are prepared as `Visual Studio Community <https://visualstudio.microsoft.com/vs/community/>`_ projects.
You can directly open project in the IDE, compile & debug.

Application opens *COM* port, set in the low-level driver.
External USB to UART converter (FTDI-like device) is necessary in order to connect to *ESP* device.

.. note::
	*ESP* device is connected with *USB to UART converter* only by *RX* and *TX* pins.

Device driver is located in ``/lwesp/src/system/lwesp_ll_win32.c``

STM32
*****

Embedded market is supported by many vendors and STMicroelectronics is, with their `STM32 <https://www.st.com/en/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus.html>`_ series of microcontrollers, one of the most important players.
There are numerous amount of examples and topics related to this architecture.

Examples for *STM32* are natively supported with `STM32CubeIDE <https://www.st.com/en/development-tools/stm32cubeide.html>`_, an official development IDE from STMicroelectronics.

You can run examples on one of official development boards, available in repository examples.

.. table:: Supported development boards

	+----------------------+--------------------------------------------------+----------------------+
	| Board name           | ESP settings                                     | Debug settings       |
	|                      +--------+------+------+------+------+------+------+--------+------+------+
	|                      | UART   | MTX  | MRX  | RST  | GP0  | GP2  | CHPD | UART   | MDTX | MDRX |
	+======================+========+======+======+======+======+======+======+========+======+======+
	| STM32F769I-Discovery | UART5  | PC12 | PD2  | PJ14 | -    | -    | -    | USART1 | PA9  | PA10 |
	+----------------------+--------+------+------+------+------+------+------+--------+------+------+
	| STM32F723E-Discovery | UART5  | PC12 | PD2  | PG14 | -    | PD6  | PD3  | USART6 | PC6  | PC7  |
	+----------------------+--------+------+------+------+------+------+------+--------+------+------+
	| STM32L496G-Discovery | USART1 | PB6  | PG10 | PB2  | PH2  | PA0  | PA4  | USART2 | PA2  | PD6  |
	+----------------------+--------+------+------+------+------+------+------+--------+------+------+
	| STM32L432KC-Nucleo   | USART1 | PA9  | PA10 | PA12 | PA7  | PA6  | PB0  | USART2 | PA2  | PA3  |
	+----------------------+--------+------+------+------+------+------+------+--------+------+------+
	| STM32F429ZI-Nucleo   | USART2 | PD5  | PD6  | PD1  | PD4  | PD7  | PD3  | USART3 | PD8  | PD9  |
	+----------------------+--------+------+------+------+------+------+------+--------+------+------+

Pins to connect with ESP device:

* *MTX*: MCU TX pin, connected to ESP RX pin
* *MRX*: MCU RX pin, connected to ESP TX pin
* *RST*: MCU output pin to control reset state of ESP device
* *GP0*: `GPIO0` pin of ESP8266, connected to MCU, configured as output at MCU side
* *GP2*: `GPIO2` pin of ESP8266, connected to MCU, configured as output at MCU side
* *CHPD*: `CH_PD` pin of ESP8266, connected to MCU, configured as output at MCU side

.. note::
	*GP0*, *GP2*, *CH_PD* pins are not always necessary for *ESP* device to work properly.
	When not used, these pins must be tied to fixed values as explained in *ESP* datasheet.

Other pins are for your information and are used for debugging purposes on board.

* MDTX: MCU Debug TX pin, connected via on-board ST-Link to PC
* MDRX: MCU Debug RX pin, connected via on-board ST-Link to PC
* Baudrate is always set to ``921600`` bauds

Examples list
^^^^^^^^^^^^^

Here is a list of all examples coming with this library.

.. tip::
	Examples are located in ``/examples/`` folder in downloaded package.
	Check :ref:`download_library` section to get your package.

.. warning::
	Several examples need to connect to access point first, then they may start client connection or pinging server.
	Application needs to modify file ``/snippets/station_manager.c`` and update ``ap_list`` variable with preferred access points,
	in order to allow *ESP* to connect to home/local network

Ex. Access point
****************

*ESP* device is configured as software access point, allowing stations to connect to it.
When station connects to access point, it will output its *MAC* and *IP* addresses.

Ex. Client
**********

Application tries to connect to custom server with classic, event-based API.
It starts concurrent connections and processes data in its event callback function.

Ex. Server
**********

It starts server on port ``80`` in event based connection mode.
Every client is processed in callback function.

When *ESP* is successfully connected to access point, it is possible to connect to it using its assigned IP address.

Ex. Domain name server
**********************

*ESP* tries to get domain name from specific domain name, ``example.com`` as an example.
It needs to be connected to access point to have access to global internet.

Ex. MQTT Client
***************

This example demonstrates raw MQTT connection to mosquitto test server.
A new application thread is started after *ESP* successfully connects to access point.
MQTT application starts by initiating a new TCP connection.

This is event-based example as there is no linear code.

Ex. MQTT Client API
*******************

Similar to *MQTT Client* examples, but it uses separate thread to process
events in blocking mode. Application does not use events to process data,
rather it uses blocking API to receive packets

Ex. Netconn client
******************

Netconn client is based on sequential API.
It starts connection to server, sends initial request and then waits to receive data.

Processing is in separate thread and fully sequential, no callbacks or events.

Ex. Netconn server
******************

Netconn server is based on sequential API.
It starts server on specific port (see example details) and it waits for new client in separate threads.
Once new client has been accepted, it waits for client request and processes data accordingly by sending reply message back.

.. tip::
	Server may accept multiple clients at the same time

.. toctree::
	:maxdepth: 2
