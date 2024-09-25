#!/bin/python

import struct
from time import sleep
import datetime
import fcntl
import serial
import os
import sys
from threading import Thread, Lock

from pathlib import Path

TUNSETIFF = 0x400454CA
TUNSETOWNER = TUNSETIFF + 2
IFF_TUN = 0x0001
IFF_TAP = 0x0002
IFF_NO_PI = 0x1000
MAC_LEN = 6

MSG_DEVINFO_V2 = 0
MSG_CLIENTCONFIG_V2 = 6
MSG_PACKET_V2 = 7

INTRON = b"UN\x00\x01\x02\x03\x04\x05"
INTERFACE = "tap0"
SERIAL = sys.argv[1] if len(sys.argv) == 2 else "/dev/ttyUSB0"
BAUD_RATE = 4600000  #921600
SSID = "esptest"
PASS = "lwesp8266"
MTU = 1420

# tap = Path('/dev/net/tun').open('r+b')
tap = os.open("/dev/net/tun", 0x2)

ifr = struct.pack(b"16sH", INTERFACE.encode("ascii"), IFF_TAP | IFF_NO_PI)
fcntl.ioctl(tap, TUNSETIFF, ifr)
fcntl.ioctl(tap, TUNSETOWNER, os.getuid())

ser = serial.Serial(SERIAL, baudrate=BAUD_RATE, parity=serial.PARITY_NONE)
last_in = datetime.datetime.now()
lock = Lock()


def safe(b: bytes):
    try:
        return b.decode("ascii")
    except Exception:
        return b


link_up = False


def wait_for_intron():
    # print("TAP: Waiting for intron")
    pos = 0
    while pos < len(INTRON):
        c = ser.read(1)[0]
        if c == INTRON[pos]:
            pos = pos + 1
            # print(f"TAP: INTRON: pos: {pos}, byte: {bytes([c])}")
            #print("I", end="", flush=True)
        else:
            print(f"{safe(bytes([c]))}", end="")
            pos = 0
            #print("X", end="", flush=True)
    # print("TAP: intron found")


def recv_link(up_data):
    global link_up
    up = int.from_bytes(up_data, byteorder='big', signed=False) == 1
    link_up = up
    word = "up" if up else "down"
    print(f"TAP: Setting link {word}")
    os.system(f"ip link set {INTERFACE} {word}")

    if not up:
        send_wifi_client()


def recv_packet():
    global last_in
    up = ser.read(1)
    recv_link(up_data)
    size = int.from_bytes(ser.read(2), byteorder='big', signed=False)
    if size:
        packet = ser.read(size)
        # print(f"SIN : {packet.hex()}")
        try:
            os.write(tap, packet)
            with lock:
                last_in = datetime.datetime.now()
        except IOError:
            print("TAP: FAILED TO WRITE")


def send_wifi_client():
    print(f"TAP: Sending client config:  ssid: {SSID}, pass: {PASS}")

    ssid_data = SSID.encode()
    pass_data = PASS.encode()
    ssid_part = len(ssid_data).to_bytes(
        length=1, byteorder='big', signed=False) + ssid_data
    pass_part = len(pass_data).to_bytes(
        length=1, byteorder='big', signed=False) + pass_data
    payload = INTRON + ssid_part + pass_part

    send_message(MSG_CLIENTCONFIG_V2, 0, payload)


send_lock = Lock()


def send_message(msg_type, msg_byte, payload: bytes):
    with send_lock:
        ser.write(INTRON +
                  msg_type.to_bytes(1, byteorder='big', signed=False) +
                  msg_byte.to_bytes(1, byteorder='big', signed=False) +
                  len(payload).to_bytes(2, byteorder='big', signed=False) +
                  payload)
        try:
            ser.flush()
        except Exception:
            print("Flush failed")


def recv_devinfo():
    # ESP FW version
    version = int.from_bytes(ser.read(2), byteorder='big', signed=False)
    size = int.from_bytes(ser.read(2), byteorder='big', signed=False)
    print(f"TAP: ESP FW version: {version}")

    mac = ser.read(MAC_LEN)
    print(f"TAP: Device info mac: {mac.hex(' ')}")
    print(f"TAP: ip link set {INTERFACE} address {mac.hex(':')}")
    os.system(f"ip link set {INTERFACE} address {mac.hex(':')}")
    print(f"ip link set {INTERFACE} mtu {MTU}")
    os.system(f"ip link set {INTERFACE} mtu {MTU}")


def recv_message():
    wait_for_intron()
    type_data = ser.read(1)
    # print(f"TAP: Receiving message type: {type_data}")

    type_value = int.from_bytes(type_data, byteorder='big', signed=False)
    if type_value == MSG_PACKET_V2:
        recv_packet()
    elif type_value == MSG_DEVINFO_V2:
        recv_devinfo()
    else:
        print(f"TAP: Unknown message type: {type_value}")


def serial_thread():
    while True:
        try:
            recv_message()
        except:
            pass


Thread(target=serial_thread, daemon=True).start()


def ping_thread():
    while True:
        sleep(30)
        print("###### Sending getlink")
        send_message(MSG_PACKET_V2, 0, b'')


Thread(target=ping_thread, daemon=True).start()

print("TAP: Configuring wifi")
send_wifi_client()

print("TAP: Reading tap device")
while True:
    packet = os.read(tap, 1024)
    # print(f"TAP: SOUT: {packet.hex()}, LEN: {len(packet)}")

    send_message(MSG_PACKET_V2, 0, packet)
    #print("O", end="", flush=True)
    # with lock:
    #     if (datetime.datetime.now() - last_in).total_seconds() > 5:
    #         print("UART reset")
    #         ser.close()
    #         ser = serial.Serial("/dev/ttyUSB0", baudrate=BAUD_RATE, parity=serial.PARITY_NONE)

tap.close()
