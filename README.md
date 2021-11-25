# WiFi NIC

## How to run this

This implements Linux Wifi NIC on top of UART and ESP8266

- Make sure your Linux supports tap devices
- Compile this
- Flash the binary into ESP
- Tune constants in tap/tap.py
- Run tap/tap.py 
- Check tap device was created
- (Run dhcp client on tap device)
- Check this works as (terribly slow) network interface


## Configure the project

```
idf.py menuconfig
```

* Set serial port under Serial Flasher Options.

* Set WiFi SSID and WiFi Password and Maximum retry under Example Configuration Options.

## Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

```
