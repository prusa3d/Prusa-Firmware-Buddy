.. _um_overview:

Overview
========

WiFi devices (focus on *ESP8266* and *ESP32*) from *Espressif Systems* are low-cost and very useful for embedded projects.
These are classic microcontrollers without embedded flash memory. Application needs to assure external Quad-SPI flash to execute code from it directly.

*Espressif* offers SDK to program these microcontrollers directly and run code from there.
It is called *RTOS-based SDK*, written in C language, and allows customers to program MCU starting with ``main`` function.
These devices have some basic peripherals, such as GPIO, ADC, SPI, I2C, UART, etc. Pretty basic though.

Wifi connectivity is often part of bigger system with more powerful MCU.
There is usually bigger MCU + Wifi transceiver (usually module) aside with UART/SPI communication.
MCU handles application, such as display & graphics, runs operating systems, drives motor and has additional external memories.

.. figure:: ../static/images/example_app_arch.svg
	:align: center
	:alt: Typical application example with access to WiFi

	Typical application example with access to WiFi

*Espressif* is not only developing *RTOS SDK* firmware, it also develops *AT Slave firmware* based on *RTOS-SDK*.
This is a special application, which is running on *ESP* device and allows host MCU to send *AT commands* and get response for it.
Now it is time to use *LwESP* you are reading this manual for.

*LwESP* has been developed to allow customers to:

* Develop on single (host MCU) architecture at the same time and do not care about *Espressif* arch
* Shorten time to market

Customers using *LwESP* do not need to take care about proper command for specific task,
they can call API functions, such as :cpp:func:`lwesp_sta_join` to join WiFi network instead.
Library will take the necessary steps in order to send right command to device via low-level driver (usually UART) and
process incoming response from device before it will notify application layer if it was successfuly or not.

.. note::
	*LwESP* offers efficient communication between host MCU at one side and *Espressif* wifi transceiver on another side.

To summarize:

* *ESP* device runs official *AT* firmware, provided by *Espressif systems*
* Host MCU runs custom application, together with *LwESP* library
* Host MCU communicates with *ESP* device with UART or similar interface.

.. toctree::
    :maxdepth: 2
    :glob: