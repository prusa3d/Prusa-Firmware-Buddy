# ESP8266 AT commands parser for RTOS systems

ESP-AT Library commands parser is a generic, platform independent, library for communicating with ESP8266 Wi-Fi module using AT commands. Module is written in C99 and is independent from used platform. Its main targets are embedded system devices like ARM Cortex-M, AVR, PIC and others, but can easily work under `Windows`, `Linux` or `MAC` environments.

`ESP_AT_Lib` is a library for host device (MCU for example) which drivers ESP32 or ESP8266 devices which run official `AT commands` firmware written and maintained by Espressif System.
Source of official firmware is publicly available in [esp32-at](https://github.com/espressif/esp32-at) repository [Although name might be confusing, it supports ESP32 and ESP8266 aswell].
Official ESP AT firmware is based on Espressif RTOS-SDK.

If you are only interested in using module and not writing firmware for it, you may download pre-build AT firmware from [Espressif systems download section](https://www.espressif.com/en/support/download/at) and focus only on firmware development on host side, for which you need the library posted here.

Follow documentation for more information on implementation and other details.

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
- Embeds other AT features, such as `WPS`, `DNS` and others
- User friendly MIT license

## Documentation

Full API documentation with description and examples is available and is regulary updated with the source changes

http://majerle.eu/documentation/esp_at/html/index.html

## Contribution

I invite you to give feature request or report a bug. Please use issues tracker.
