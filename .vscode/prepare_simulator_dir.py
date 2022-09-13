import pathlib
import sys
import shutil

assert len(sys.argv) == 3, f'Usage: {sys.argv[0]} <build_dir> <simulator_dir>'
build_dir = pathlib.Path(sys.argv[1])
simulator_dir = pathlib.Path(sys.argv[2])
usb_dir = simulator_dir / 'usbdir'


def ensure_memory_file_exists(path, size):
    if path.exists():
        return
    with open(path, 'wb') as f:
        f.seek(size - 1)
        f.write(b'\x00')


simulator_dir.mkdir(parents=True, exist_ok=True)
usb_dir.mkdir(exist_ok=True)
shutil.copy(build_dir / 'firmware.bbf', usb_dir / 'firmware.bbf')
ensure_memory_file_exists(simulator_dir / 'xflash.bin', 2**23)
ensure_memory_file_exists(simulator_dir / 'eeprom_bank1.bin', 2**16)
ensure_memory_file_exists(simulator_dir / 'eeprom_bank2.bin', 2**16)
