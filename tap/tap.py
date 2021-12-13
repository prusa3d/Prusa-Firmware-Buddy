#!/bin/python

import struct
from time import sleep
import fcntl
import serial
import os
from threading import Thread

from pathlib import Path

TUNSETIFF = 0x400454CA
TUNSETOWNER = TUNSETIFF + 2
IFF_TUN = 0x0001
IFF_TAP = 0x0002
IFF_NO_PI = 0x1000

MSG_DEVINFO = 0
MSG_LINK = 1
MSG_CLIENTCONFIG = 2
MSG_PACKET = 3

INTRON = b"UNU"
INTERFACE = "tap0"
BAUD_RATE = 927600
SSID = "ssid"
PASS = "pass"

# tap = Path('/dev/net/tun').open('r+b')
tap = os.open("/dev/net/tun", 0x2)

ifr = struct.pack(b"16sH", INTERFACE.encode("ascii"), IFF_TAP | IFF_NO_PI)
fcntl.ioctl(tap, TUNSETIFF, ifr)
fcntl.ioctl(tap, TUNSETOWNER, os.getuid())


ser = serial.Serial("/dev/ttyUSB0", BAUD_RATE)


def safe(b: bytes):
    try:
        return b.decode("ascii")
    except Exception:
        return b


def wait_for_intron():
    # print("TAP: Waiting for intron")
    pos = 0
    while pos < len(INTRON):
        c = ser.read(1)[0]
        if c == INTRON[pos]:
            pos = pos + 1
            # print(f"TAP: INTRON: pos: {pos}, byte: {bytes([c])}")
        else:
            print(f"{safe(bytes([c]))}", end="")
            pos = 0
    # print("TAP: intron found")


def recv_packet():
    len_data = ser.read(4)
    len = int.from_bytes(len_data, "little", signed=False)
    # print(f"TAP: packet len: {len}, data: {len_data.hex()}")
    packet = ser.read(len)
    # print(f"SIN : {packet.hex()}")
    try:
        os.write(tap, packet)
    except IOError:
        print("TAP: FAILED TO WRITE")


def send_wifi_client():
    print(f"TAP: Sending client config:  ssid: {SSID}, pass: {PASS}")
    ser.write(INTRON + MSG_CLIENTCONFIG.to_bytes(1, "little"))
    ssid_data = SSID.encode()
    ser.write(len(ssid_data).to_bytes(length=1, byteorder="little", signed=False) + ssid_data)
    pass_data = PASS.encode()
    ser.write(len(pass_data).to_bytes(length=1, byteorder="little", signed=False) + pass_data)
    ser.flush()

link_up = False

def recv_link():
    up_data = ser.read(1)
    up = int.from_bytes(up_data, "little", signed=False) == 1
    link_up = up
    word = "up" if up else "down"
    print(f"TAP: Setting link {word}")
    os.system(f"ip link set {INTERFACE} {word}")

    if not up:
        send_wifi_client()


def recv_devinfo():
    mac_len_data = ser.read(1)
    mac_len = int.from_bytes(mac_len_data, "little", signed=False)
    mac = ser.read(mac_len)
    print(f"TAP: Device info mac: {mac.hex(' ')}")
    print(f"TAP: ip link set {INTERFACE} address {mac.hex(':')}")
    os.system(f"ip link set {INTERFACE} address {mac.hex(':')}")


def recv_message():
    wait_for_intron()
    type_data = ser.read(1)
    # print(f"TAP: Receiving message type: {type_data}")

    type_value = int.from_bytes(type_data, "little", signed=False)
    if type_value == MSG_PACKET:
        recv_packet()
    elif type_value == MSG_LINK:
        recv_link()
    elif type_value == MSG_DEVINFO:
        recv_devinfo()
    else:
        print(f"TAP: Unknown message type: {type_value}")


def serial_thread():
    while True:
        recv_message()


Thread(target=serial_thread, daemon=True).start()


print("TAP: Configuring wifi")
send_wifi_client()

print("TAP: Reading tap device")
while True:
    packet = os.read(tap, 2048)
    # print(f"TAP: SOUT: {packet.hex()}, LEN: {len(packet)}")

    # if not link_up:
    #     send_wifi_client()

    out = (
        INTRON
        + MSG_PACKET.to_bytes(1, "little")
        + len(packet).to_bytes(4, "little", signed=False)
        + packet
    )
    # print(f"TAP: SOUT MESSAGE: {out}")
    ser.write(out)
    ser.flush()

tap.close()
