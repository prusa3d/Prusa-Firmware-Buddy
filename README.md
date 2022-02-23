# WiFi NIC

## How to run this

This implements Wifi NIC on top of UART and ESP8266. The NIC can be used either on Linux using provided script or attached to another MCU and integrated with LwIP as network interface.

## Build environment

Either build docker image as described by provided Dockerfile or proceed according to the [ESP8266 RTOS SDK user manual](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html)

## Build

This assumes SDK environemnt was sources and current directory is checked out root of the project.

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py all
```

## Flash from printer

Create ESP directory in the root of the USB flash drive. Copy following files to it:

- `build/partition_table/partition-table.bin`
- `build/bootloader/bootloader.bin`
- `build/uart_wifi.bin`

Insert it into printer and start ESP flash from the menu.

## Flash using USB "programmer"

Reset ESP with IO0 down and issue the following command.

```
idf.py -p /dev/ttyXXX flash
```

## Test on Linux

- Make sure your Linux supports tap devices
- Compile this
- Flash the binary into ESP
- Tune constants in tap/tap.py
- Run tap/tap.py
- Check tap device was created
- (Run dhcp client on tap device)
- Check this works as (terribly slow) network interface