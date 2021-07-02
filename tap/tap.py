#!/bin/python

import struct
from time import sleep
import fcntl
import serial
import os
from threading import Thread

from pathlib import Path

TUNSETIFF = 0x400454ca
TUNSETOWNER = TUNSETIFF + 2
IFF_TUN = 0x0001
IFF_TAP = 0x0002
IFF_NO_PI = 0x1000

iface = b"tap0"

#tap = Path('/dev/net/tun').open('r+b')
tap = os.open('/dev/net/tun', 0x2)

ifr = struct.pack(b'16sH', iface, IFF_TAP | IFF_NO_PI)
fcntl.ioctl(tap, TUNSETIFF, ifr)
fcntl.ioctl(tap, TUNSETOWNER, 1000)


os.system("ip link set tap0 address bc:dd:c2:6b:61:82")
os.system("ip link set tap0 up")


ser = serial.Serial('/dev/ttyUSB0', 9600)


def serial_thread():
    CMD = b"AT+INPUT:"
    CMD_LEN = 9
    state = 0

    while True:
        c = ser.read(1)[0]

        if state < CMD_LEN:
            if CMD[state] == c:
                state += 1
                if state != CMD_LEN:
                    # print(f"STATE: {state}, CHAR: {bytes([c])}\n")
                    continue
            else:
                if CMD[0] == c:
                    state = 1
                else:
                    # print(f"!!!STATE: {state}, CHAR: {bytes([c])}\n")
                    try:
                        print(f"{bytes([c]).decode()}", end='')
                    except:
                        pass
                    state = 0

        # print("READIN LEN")
        if state == CMD_LEN:
            len = 0
            c = None

            while c != b','[0]:
                c = ser.read(1)[0]
                if b'0'[0] <= c <= b'9'[0]:
                    len = len * 10 + c - b'0'[0]

            # print(f"READING PACKET LEN: {len}\n")

            packet = ser.read(len)
            print(f"SIN : {packet.hex()}")
            try:
                os.write(tap, packet)
            except IOError:
                print("FAILED TO WRITE")
            state = 0


Thread(target=serial_thread, daemon=True).start()

print("Reading tap device")
while True:
    packet = os.read(tap, 2048)
    print(f"SOUT: {packet.hex()}")

    out = b"\nAT+OUTPUT:" + str(len(packet)).encode() + b"," + packet
    # print(f"SOUT MESSAGE: {out}")
    ser.write(out)
    ser.flush()

tap.close()

