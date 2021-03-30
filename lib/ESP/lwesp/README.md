# Lightweight ESP-AT parser

LwESP is lightweight ESP AT commands parser library to communicate with ESP8266 or ESP32 Wi-Fi module using AT commands. Module is written in C99 and is independent from used platform. Its main targets are embedded system devices like ARM Cortex-M, AVR, PIC and others, but can easily work under `Windows`, `Linux` or `MAC` environments.

`esp-at-lib` is a library for host device (MCU as an example) which drives ESP32 or ESP8266 devices running official `AT commands` firmware written and maintained by Espressif System.
Source of official firmware is publicly available at [esp32-at](https://github.com/espressif/esp-at) repository.

If you are only interested in using module and not writing firmware for it, you may download pre-build AT firmware from [Espressif systems download section](https://www.espressif.com/en/support/download/at) and focus only on firmware development on host side, for which you need the library posted here.

Follow documentation for more information on implementation and details.

<h3>Read first: <a href="https://docs.majerle.eu/projects/lwesp/">Documentation</a></h3>

## Features

- Supports latest ESP32 & ESP8266 RTOS-SDK based AT commands firmware
- Platform independent and very easy to port
- Development of library under Win32 platform
- Available examples for ARM Cortex-M or Win32 platforms
- Written in C language (C99)
- Allows different configurations to optimize user requirements
- Supports implementation with operating systems with advanced inter-thread communications
- Uses `2` tasks for data handling from user and device
- Includes several applications built on top of library
  - Netconn sequential API for client and server
  - HTTP server with dynamic files (file system) supported
  - MQTT client
- Embeds other AT features, such as WPS management, custom DNS setup, Hostname for DHCP, Ping feature
- User friendly MIT license

## Contribute

Fresh contributions are always welcome. Simple instructions to proceed::

1. Fork Github repository
2. Respect [C style & coding rules](https://github.com/MaJerle/c-code-style) used by the library
3. Create a pull request to develop branch with new features or bug fixes

Alternatively you may:

1. Report a bug
2. Ask for a feature request
