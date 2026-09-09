# WiFi NIC firmware

Printers get WiFi through an ESP module connected over UART.
The ESP runs a minimal firmware that acts as a network card.
It forwards raw ethernet frames in both directions, reports link status and performs AP scans.
The whole IP stack stays on the printer, in LwIP.
The printer side of the link is `lib/WUI/espif.cpp`.
Both ends implement the V2 wire protocol, documented in [protocol.txt](protocol.txt).

Firmware for both esp8266 and esp32 is located at `lib/esp-nic/`:

| | SDK | Printers |
|---|---|---|
| ESP8266 | ESP8266\_RTOS\_SDK | MK4, MK3.5, MINI, COREONE (all `HAS_ESP` except XL) |
| ESP32 | ESP-IDF v5.0.3 | XL (`HAS_EMBEDDED_ESP32`) |

> The code was originally developed in separate repositories:
> - ESP8266: [prusa3d/Prusa-ESP-Nic-FW](https://github.com/prusa3d/Prusa-ESP-Nic-FW)
> - ESP32: [prusa3d/Prusa-ESP32-Nic-FW](https://github.com/prusa3d/Prusa-ESP32-Nic-FW)
