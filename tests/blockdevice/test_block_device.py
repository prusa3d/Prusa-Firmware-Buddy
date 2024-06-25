import pytest
import os
import random

BLOCK_SIZE = 4096
BLOCK_COUNT = 1900


@pytest.fixture
def block_device_path(pytestconfig):
    return pytestconfig.getoption('--device')


@pytest.fixture
def dev(block_device_path):
    with open(block_device_path, 'br+', buffering=0) as dev:
        yield dev


def select_random_block():
    return random.randint(0, BLOCK_COUNT)


@pytest.mark.benchmark(min_rounds=5, warmup=False)
def test_block_read_write_speed(benchmark, dev):

    def perform_read_write():
        block = select_random_block()
        data = os.urandom(BLOCK_SIZE)
        dev.seek(block * BLOCK_SIZE)
        dev.write(data)
        dev.seek(block * BLOCK_SIZE)
        return data == dev.read(BLOCK_SIZE)

    assert benchmark(perform_read_write)


def test_block_read_10_blocks_speed(benchmark, dev):

    def perform_read():
        block = select_random_block()
        dev.seek(block * BLOCK_SIZE)
        dev.read(BLOCK_SIZE)

    benchmark(perform_read)


def test_data_consistency(dev):
    length = BLOCK_SIZE * random.randint(2, 10)
    offset = BLOCK_SIZE * random.randint(0, 100)
    data = os.urandom(length)
    dev.seek(offset)
    dev.write(data)
    dev.seek(offset + BLOCK_SIZE)
    read_back = dev.read(length - BLOCK_SIZE)
    assert data[BLOCK_SIZE:] == read_back


generate_data_idx = 0


def generate_data(length):
    global generate_data_idx
    data = bytearray()
    while len(data) < length:
        data.extend(f'  {generate_data_idx:010}'.encode('utf-8'))
        generate_data_idx += 1
    return data[:length]


def test_large_data_read_write(dev, tmpdir):
    data = bytearray(generate_data(BLOCK_COUNT * BLOCK_SIZE))
    dev.seek(0)
    dev.write(data)

    tmpdir.join('initial').write_binary(data)

    dev.seek(0)
    initial_read_back = dev.read(len(data))
    tmpdir.join('initial-readback').write_binary(initial_read_back)
    assert data == initial_read_back

    for _ in range(1):
        # make random change
        length = random.randint(1, 3) * BLOCK_SIZE
        offset = random.randint(
            0,
            len(data) // BLOCK_SIZE - (length // BLOCK_SIZE) - 1) * BLOCK_SIZE
        print(f'changing offset {offset} length {length}')
        data_to_replace = generate_data(length)
        dev.seek(offset)
        dev.write(data_to_replace)
        data[offset:offset + length] = bytearray(data_to_replace)

    dev.seek(0)
    read_back = dev.read(len(data))

    tmpdir.join('expected').write_binary(data)
    tmpdir.join('final').write_binary(read_back)

    assert data == read_back
