#!/usr/bin/env python3

from collections import namedtuple
from contextlib import closing
from enum import Enum
from random import randbytes
from time import sleep
import serial

# This is a tool for communicating with puppies without the printer.
# Puppies are XL/iX daughterboards communicating using MODBUS over RS485.
# Right now only flashing the modularbed is implemented.
# Feel free to extend this as needed.

# Note on serial/USB converters for computers without physical serial port:
# do not use cheap RS485 converters like YYH-256 because they toggle pins
# like crazy, so you will always read garbage. Read more at
# https://hackaday.io/project/167532-modbus-things-with-stm8-eforth/log/168474-finding-the-culprit
# More expensive converters like FTDI's USB-COM485-PLUS1 are working flawlesly.


def crc_modbus(data):
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for i in range(8):
            if crc & 1:
                crc >>= 1
                crc ^= 0xA001
            else:
                crc >>= 1
    return crc


def parse_int_modbus(data):
    return int.from_bytes(data, byteorder='big')


MAX_WRITE_CHUNK = 247


class Bus(object):

    def __init__(self, port, address: bytes):
        assert (len(address) == 1)
        self.address = address
        timeout = 0.02  #shorter timeouts were giving me troubles
        self._serial = serial.Serial(port,
                                     baudrate=230400,
                                     stopbits=serial.STOPBITS_ONE,
                                     parity=serial.PARITY_NONE,
                                     timeout=timeout)

    def close(self):
        self._serial.close()

    def transaction(self, tx_pdu, wait=None):
        packet_nocrc = self.address + tx_pdu
        tx_adu = packet_nocrc + crc_modbus(packet_nocrc).to_bytes(2, 'little')
        self._serial.write(tx_adu)
        self._serial.flush()

        if wait:
            sleep(wait)

        rx_adu = self._serial.read(256)

        if crc_modbus(rx_adu) != 0:
            raise Exception(f'crc mismatch, got {rx_adu}')

        if rx_adu[0:1] != self.address:
            raise Exception('address mismatch')

        rx_pdu = rx_adu[1:-2]
        return rx_pdu


HardwareInfo = namedtuple('HardwareInfo',
                          ['hw_type', 'hw_revision', 'bl_version', 'app_size'])


class Hardware(Enum):
    MODULARBED = 43
    DWARF = 44
    XBUDDY_EXTENSION = 45


class MessageId(Enum):
    GET_PROTOCOL_VERSION = b'\x00'
    SET_ADDRESS = b'\x01'
    GET_HARDWARE_INFO = b'\x03'
    START_APPLICATION = b'\x05'
    WRITE_FLASH = b'\x06'
    FINALIZE_FLASH = b'\x07'
    READ_FLASH = b'\x08'
    GET_MAX_PACKET_LENGTH = b'\x0c'
    GET_FINGERPRINT = b'\x0e'
    COMPUTE_FINGERPRINT = b'\x0f'
    READ_OTP = b'\x10'


class BootloaderProtocol(object):
    '''This class builds on top of Bus and provides low-level bootloader specific functions.'''

    def __init__(self, bus):
        self._bus = bus

    def _transaction(self, tx_pdu: bytes, wait=None):
        # this is like modbus, but with custom function codes and error handling
        rx_pdu = self._bus.transaction(tx_pdu, wait)
        status = rx_pdu[0:1]
        length = rx_pdu[1:2]
        result = rx_pdu[2:]

        if status != b'\x00':
            raise Exception(
                f'status mismatch, got {status}, {length}, {result}')

        if parse_int_modbus(length) != len(result):
            raise Exception('length mismatch')

        return result

    def get_protocol_version(self):
        return self._transaction(MessageId.GET_PROTOCOL_VERSION.value)

    def get_hardware_info(self):
        payload = self._transaction(MessageId.GET_HARDWARE_INFO.value)
        if len(payload) != 11:
            raise Exception('length mismatch')

        return HardwareInfo(hw_type=Hardware(parse_int_modbus(payload[0:1])),
                            hw_revision=parse_int_modbus(payload[1:3]),
                            bl_version=parse_int_modbus(payload[3:7]),
                            app_size=parse_int_modbus(payload[7:11]))

    def set_address(self, new_address: bytes):
        assert (len(new_address) == 1)
        self._transaction(MessageId.SET_ADDRESS.value + new_address)

    def read_flash(self, address: int, read_len: int):
        return self._transaction(MessageId.READ_FLASH.value +
                                 address.to_bytes(4, byteorder='big') +
                                 read_len.to_bytes(1))

    def read_otp(self, address: int, read_len: int):
        return self._transaction(MessageId.READ_OTP.value +
                                 address.to_bytes(4, byteorder='big') +
                                 read_len.to_bytes(1))

    def get_max_packet_length(self):
        return parse_int_modbus(
            self._transaction(MessageId.GET_MAX_PACKET_LENGTH.value))

    def start_application(self, salt, fingerprint):
        return self._transaction(MessageId.START_APPLICATION.value + salt +
                                 fingerprint)

    def write_flash(self, address: int, data: bytes):
        self._transaction(MessageId.WRITE_FLASH.value +
                          address.to_bytes(4, byteorder='big') + data)

    def finalize_flash(self):
        self._transaction(MessageId.FINALIZE_FLASH.value)

    def get_fingerprint(self):
        return self._transaction(MessageId.GET_FINGERPRINT.value)

    def compute_fingerprint(self, salt):
        self._transaction(MessageId.COMPUTE_FINGERPRINT.value + salt, wait=0.5)


class Bootloader(object):
    '''This class builds on top of BootloaderProtocol and provides higher-level functions.'''

    def __init__(self, bus):
        self._protocol = BootloaderProtocol(bus)

    def is_running(self):
        try:
            protocol_version = self._protocol.get_protocol_version()
            return parse_int_modbus(protocol_version) == 0x0302
        except Exception as ex:
            return False

    def flash_firmware_blob(self, blob):
        # TODO avoid flashing if already flashed
        offset = 0
        while offset < len(blob):
            chunk = blob[offset:offset + MAX_WRITE_CHUNK]
            self._protocol.write_flash(offset, chunk)
            offset += MAX_WRITE_CHUNK
        self._protocol.finalize_flash()

    def flash_firmware_path(self, path):
        with open(path, 'rb') as f:
            self.flash_firmware_blob(f.read())

    def run_application(self):
        salt = randbytes(4)
        self._protocol.compute_fingerprint(salt)
        fingerprint = self._protocol.get_fingerprint()
        # TODO dwarf may require changing address before starting application
        started = self._protocol.start_application(salt, fingerprint)
        if started != b'\x01':
            raise Exception('start_application failed')
        sleep(1)


class ModbusProtocol(object):
    '''This class builds on top of Bus and provides modbus specific functions.'''

    def __init__(self, bus):
        self._bus = bus

    def _transaction(self, tx_pdu, wait=None):
        rx_pdu = self._bus.transaction(tx_pdu, wait)
        return rx_pdu

    def read_input_registers(self, address, length):
        return self._transaction(b'\x04' + address.to_bytes(2, 'big') +
                                 length.to_bytes(2, 'big'))


def main():
    port = '/dev/ttyUSB0'
    modularbed_firmware_path = 'build-vscode-modularbed/firmware.bin'
    with closing(Bus(port, b'\x00')) as bus:
        bootloader = Bootloader(bus)
        if not bootloader.is_running():
            print('reset the board, bootloader needs to be running')
            return

        hardware_info = bootloader._protocol.get_hardware_info()
        if hardware_info.hw_type == Hardware.MODULARBED:
            print('flashing modular bed, this may take a while')
            bootloader.flash_firmware_path(modularbed_firmware_path)
            print('running modular bed')
            bootloader.run_application()

        else:
            raise Exception('unknown hw_type {hardware_info.hw_type}')

        # from now on, board communicates using modbus, for example:
        modbus_protocol = ModbusProtocol(bus)
        values = modbus_protocol.read_input_registers(0x8001, 16)
        print(values)


if __name__ == '__main__':
    main()
    exit(0)
