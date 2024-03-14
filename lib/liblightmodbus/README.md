# A lightweight, hardware-agnostic, Modbus RTU/TCP library
[![GitHub](https://img.shields.io/github/license/Jacajack/liblightmodbus)](https://github.com/Jacajack/liblightmodbus/blob/master/LICENSE) [![Build Status](https://img.shields.io/endpoint.svg?url=https%3A%2F%2Factions-badge.atrox.dev%2Fjacajack%2Fliblightmodbus%2Fbadge%3Fref%3Dmaster&style=flat)](https://actions-badge.atrox.dev/jacajack/liblightmodbus/goto?ref=master) [![Coveralls branch](https://img.shields.io/coveralls/github/Jacajack/liblightmodbus/master)](https://coveralls.io/github/Jacajack/liblightmodbus?branch=master) [![Donate](https://img.shields.io/badge/Donate-PayPal-blue)](https://www.paypal.com/donate?hosted_button_id=KZ7DV93D98GAL)

Liblightmodbus is a lightweight, highly configurable, hardware-agnostic Modbus RTU/TCP library written in C99.

## Features
- Modbus RTU and TCP support
- Independent from the hardware layer
- Modular structure helps to minimize the resource usage
- Callback-based operation
- User-defined memory allocator support (static memory allocation is possible)
- Support for custom Modbus functions; 01, 02, 03, 04, 05, 06, 15, 16 and 22 are implemented by default. 
- A (very) experimental C++ interface
- [ESP-IDF component](https://github.com/Jacajack/liblightmodbus-esp)

Check the [online documentation](https://jacajack.github.io/liblightmodbus/) for more technical information.

## Support
If you face any problems, please refer to the [docs](https://jacajack.github.io/liblightmodbus/) first. If you can't find answer to your question there, please [open an issue](https://github.com/Jacajack/liblightmodbus/issues/new). Hopefully this will help to form some kind of a FAQ list.
If you want to help - you can contribute on Github or [donate](https://www.paypal.com/donate/?hosted_button_id=KZ7DV93D98GAL). Both donations and contributions are very welcome :heart:

## Getting started
 - [Docs and user manual](https://jacajack.github.io/liblightmodbus/)
 - [Porting code from v2.0](https://jacajack.github.io/liblightmodbus/porting.html)
 - [Examples](./examples/)