import struct
import hashlib
import typing
import ctypes

BANK_SIZE = (8096 - 0x500) // 2
FIRST_BANK_OFFSET = 0x500

LAST_ITEM_ID = 0x0F
MAX_ITEM_SIZE = 512
ITEM_HEADER_SIZE = 3
BANK_HEADER_SIZE = 6


def crc32_sw(data, crc):
    """Gives the same results as the crc32_sw() function in src/common/crc32.h"""
    value = crc ^ 0xFFFFFFFF
    for byte in data:
        value ^= byte
        for _ in range(8):
            if value & 1:
                value = (value >> 1) ^ 0xEDB88320
            else:
                value >>= 1
    value ^= 0xFFFFFFFF
    return value


def generate_id(name):
    hasher = hashlib.sha256()
    hasher.update(name.encode('utf-8'))
    hashed_value = hasher.digest()
    slice = (hashed_value[0] << 8) | hashed_value[1]
    return slice & 0x3FFF


def generate_item_header(last_item, id, data_len):
    # ItemHeader
    # [last item] [7 lsb of id]  --  [ 7 msb of id] [1 lsb of len] -- [8 msb of len]
    first_byte = ((id & 0x7F) << 1) | (last_item & 0x01)
    second_byte = ((data_len & 0x01) << 7) | ((id >> 7) & 0x7F)
    third_byte = (data_len >> 1) & 0xFF
    data = struct.pack('<BBB', first_byte, second_byte, third_byte)
    assert len(data) == ITEM_HEADER_SIZE
    return data


def generate_bank_header(sequence_id, version):
    # BankHeader = 32 bit sequence_id, 16 bit version
    header = struct.pack('<IH', sequence_id, version)
    crc = crc32_sw(header, 0)
    return header + struct.pack('<I', crc)


def generate_transaction(items: typing.List[typing.Tuple[str, bytes]]):
    data = b''
    for i, item in enumerate(items):
        # Last item in transaction has LastItem flag set to true
        last_item = int(i == len(items) - 1)
        item_name, item_data = item
        name_hash = generate_id(item_name)
        data_len = len(item_data)

        header = generate_item_header(last_item, name_hash, data_len)
        data += header + item_data

    # Append CRC of the transaction
    crc = crc32_sw(data, 0)
    data += struct.pack('<I', crc)

    return data


def generate_bank(*, sequence_id: int, version: int,
                  items: typing.List[typing.Tuple[str, bytes]]):
    header = generate_bank_header(sequence_id, version)
    transaction = generate_transaction(items)
    bank_data = (header + transaction).ljust(BANK_SIZE, b'\xFF')
    eeprom_data = (b'\xFF' * FIRST_BANK_OFFSET) + bank_data + (b'\xFF' *
                                                               BANK_SIZE)
    return eeprom_data


if __name__ == '__main__':
    items = [
        ('Run Selftest', struct.pack('<B', False)),
        ('Run XYZ Calibration', struct.pack('<B', False)),
        ('Run First Layer', struct.pack('<B', False)),
        ('FSensor Enabled', struct.pack('<B', False)),
    ]
    bank_data = generate_bank(sequence_id=1, version=1,
                              items=items).ljust(BANK_SIZE, b'\xFF')
    eeprom_data = (b'\xFF' * FIRST_BANK_OFFSET) + bank_data + (b'\xFF' *
                                                               BANK_SIZE)
    with open(
            '/Users/alandragomirecky/Projects/prusa/Prusa-Firmware-Buddy-Private/build-vscode-buddy/simulator-mk4/Prusa_Mk4_eeprom.bin',
            'wb') as f:
        f.write(eeprom_data)
