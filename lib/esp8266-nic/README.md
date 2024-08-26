# WiFi NIC

## How to run this

This implements Wifi NIC on top of UART and ESP8266. The NIC can be used either on Linux using provided script or attached to another MCU and integrated with LwIP as network interface.

## Build environment

Either build docker image as described by provided Dockerfile or proceed according to the [ESP8266 RTOS SDK user manual](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html)

### Local build

If you want to build the Wifi NIC firmware locally you can use `setup.sh` script:

```bash
./setup.sh
```

It will intstall the ESP8266 RTOS SDK locally (it won't polute your normall environment).
It will also try to activate the local environment, to verify the instalation.

To activate the dev environment, you can just run:
```bash
source ./activate.sh
```

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
- Tune constants in tap/tap.py. Make sure that baud rate is the same in [`uart_nic.c`](https://github.com/prusa3d/Prusa-ESP-Nic-FW/blob/ad19ee3019a352415be7a1d6c2579c7bae379c35/main/uart_nic.c#L427) and `tap.py`
- Run tap/tap.py. _Note: If it doesn't print `TAP: ESP FW version: ...` or `TAP: Device info mac ...`, there's a problem with the serial connection and you may need to reduce baud rate to i.e. 1000000 and reflash._
- Check tap device was created
- Run dhcp client on tap device, i.e. `sudo dhclient tap0`
- Check this works as (terribly slow) network interface
