# RTU master application example

This directory contains an example of a Linux master application allowing to interact
with slaves connected via serial port.

Usage:  `./master <TTY> <BAUDRATE> <ADDRESS> <FUNCTION> [args]`

Function usage:
 - Function 01: `<index> <count>`
 - Function 02: `<index> <count>`
 - Function 03: `<index> <count>`
 - Function 04: `<index> <count>`
 - Function 05: `<index> <value>`
 - Function 06: `<index> <value>`
 - Function 15: `<index> [values]`
 - Function 16: `<index> [values]`
 - Function 22: `<index> <andmask> <ormask>`

Example - read 7 holding registers from slave 1 connected to `/dev/ttyUSB0` at 9600 bauds:
```
./master /dev/ttyUSB0 9600 1 3 0 7
```