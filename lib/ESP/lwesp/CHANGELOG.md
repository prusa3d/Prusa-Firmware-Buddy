# Changelog

## v1.0.0

- First stable release
- Works with *esp-at* version `v2.1.0`
- Implements all basic functionality for ESP8266 and ESP32
- Added operating system-based sequential API
- Other bug fixes and docs updates

## v0.6.1

- Fixed inadequate MQTT RX data handling causing possible overflow of memory
- Added support for zero-copy MQTT RX data

## v0.6.0

- Added support for ESP32 & ESP8266
- Official support for ESP32 AT firmware v1.2 & ESP8266 AT firmware v2.0
- Added examples to main repository
- Preparation for BLE support in ESP32
- Removed AT commands with `_CUR` and `_DEF` suffixes
- Renamed some event names, such as `ESP_EVT_CONN_CLOSE` instead of `ESP_EVT_CONN_CLOSED`
- Added DHCP/static IP support
- Added CMSIS-OS v2 support
- Added LwMEM port for dynamic memory allocation
- Other bug fixes

## v0.5.0

- Remove `_t` for every struct/enum name
- Fully use ESP_MEMCPY instead of memcpy
- When connection is in closing mode, stop sending any new data and return with error
- Remove `_data` part from event helper function for connection receive
- Implement semaphores in internal threads
- Add driver for NUCLEO-F429
- Implement timeout callback for major events when device does not reply in given time
- Add callback function support to every API function which directly interacts with device
- Replace all files to CRLF ending
- Replace `ESP_EVT_RESET` to `ESP_EVT_RESET_DETECTED`
- Replace `ESP_EVT_RESET_FINISH` to ESP_EVT_RESET`
- Replace all header files guards with ESP_HDR_ prefix
- Add espERRBLOCKING return when function is called in blocking mode when not allowed
- Other bug fixes to stabilize AT communication

## v0.4.0

- Add sizeof for every memory allocation
- Function typedefs suffix has been renamed to `_fn` instead of `_t`
- Merge events for connection data send and data send error
- Send callback if sending data is not successful in any case (timeout, ERROR, etc)
- Add functions for IP/port retrieval on connections
- Remove goto statements and use deep if statements
- Fix MQTT problems with username and password
- Make consistent variable types across library

## v1.3.0

- Rename all cb annotations with evt, replacing callbacks with events,
- Replace built-in memcpy and memset functions with `ESP_MEMCPY` and `ESP_MEMSET` to allow users to do custom implementation
- Added example for Server RTOS
- Added API to unchain first pbuf in pbuf chain
- Implemented first prototype for manual TCP receive functionality.

## v0.2.0

- Fixed netconn issue with wrong data type for OS semaphore
- Added support for asynchronous reset
- Added support for tickless sleep for modern RTOS systems

## v0.1.0

- Initial release
