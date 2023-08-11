import sys
import binascii
import functools
import asyncio
import pytest_asyncio
import hashlib
import pytest
import json
import shutil
import logging
import struct
from pathlib import Path

# add the /utils to PATH so we can use the `simulator` package
project_root = Path(__file__).resolve().parent.parent.parent
utils_dir = project_root / 'utils'
sys.path.insert(0, str(utils_dir))

from .actions import screen
from simulator import Simulator, MachineType, Printer
from persistent_stores import eeprom

logger = logging.getLogger(__name__)

DEFAULT_EEPROM_CONTENT = {
    'Run Selftest': struct.pack('<B', False),
    'Run XYZ Calibration': struct.pack('<B', False),
    'Run First Layer': struct.pack('<B', False),
    'FSensor Enabled': struct.pack('<B', False),
}


def pytest_addoption(parser):
    # yapf: disable
    parser.addoption(
        '--simulator',
        type=str,
        help='path to the simulator\'s binary to use',
    )
    parser.addoption(
        '--firmware',
        type=str,
        required=True,
        help='path to the firmware (.bin file) to test',
    )
    parser.addoption(
        '--enable-graphic',
        action='store_true',
        help='Enable graphic of QEMU to see what\'s happenning',
    )
    parser.addoption(
        '--gdb',
        action='store_true',
        help='Pass -s to QEMU to enable gdbserver',
    )
    # yapf: enable


@pytest.fixture
def simulator_path(pytestconfig) -> Path:
    path_str = pytestconfig.getoption('--simulator')
    if path_str:
        return Path(path_str)

    # use default from dependencies directory
    path = Simulator.default_simulator_path()
    if path:
        return path

    raise pytest.UsageError('failed to find default simulator'
                            '(try to run bootstrap.py) and none was'
                            'specified using the --simulator')


@pytest.fixture
def data_dir(pytestconfig) -> Path:
    return project_root / 'tests' / 'integration' / 'data'


@pytest.fixture
def firmware_path(pytestconfig):
    return Path(pytestconfig.getoption('--firmware'))


@pytest.fixture
def simulator_scriptio_port(unused_tcp_port_factory):
    return unused_tcp_port_factory()


@pytest.fixture
def simulator_proxy_port(unused_tcp_port_factory):
    return unused_tcp_port_factory()


def get_language_code(lang):
    data = [ord(c) for c in lang]
    return data[1] << 8 | data[0]


# Usage: In your module or test node use pytest.mark.parametrize('specific_eeprom_variable', DICT);
# Where DICT is desired changes or additions to DEFAULT_EEPROM_CONTENT;
# See test_prusa_link.py for example
@pytest.fixture
def specific_eeprom_variables():
    return {}


@pytest.fixture
def eeprom_variables(specific_eeprom_variables):
    DEFAULT_EEPROM_CONTENT.update(specific_eeprom_variables)
    return DEFAULT_EEPROM_CONTENT


def get_hash(*items):
    ctx = hashlib.sha256()
    for item in items:
        if isinstance(item, str):
            ctx.update(item.encode('utf-8'))
        elif isinstance(item, bytes):
            ctx.update(item)
        elif isinstance(item, Path):
            with open(item, 'rb') as f:
                for chunk in iter(lambda: f.read(1024), b''):
                    ctx.update(chunk)
        else:
            raise ValueError('invalid value type %s', type(item))
    return ctx.hexdigest()


async def prepare_xflash_content(firmware_path, basic_printer_arguments,
                                 tmpdir, flash):
    logging.info('preparing xflash content')

    # prepare eeeprom so we don't endup in wizard or somewhere else
    eeprom_dir = Path(tmpdir.mkdir('eeprom_for_xflash_init'))
    eeprom_bank_1 = eeprom_dir / 'bank1.bin'
    eeprom_bank_2 = eeprom_dir / 'bank2.bin'
    for bank in [eeprom_bank_1, eeprom_bank_2]:
        bank.write_bytes(b'\xff' * 8192)
    await prepare_eeprom_content(DEFAULT_EEPROM_CONTENT, eeprom_bank_1,
                                 eeprom_bank_2)

    # copy bbf so it can be used for bootstrap
    flash_dir = Path(tmpdir.mkdir('flash_for_xflash_init'))
    shutil.copyfile(str(firmware_path.with_suffix('.bbf')),
                    str(flash_dir / 'firmware.bbf'))

    async def wait_for_bootstrap(printer: Printer):
        text = ''
        while not all(fragment in text.lower()
                      for fragment in {'preheat', 'settings'}) and not all(
                          fragment in text.lower()
                          for fragment in {'input', 'shaper'}):
            await asyncio.sleep(1)
            text = await screen.read(printer)

    async with Simulator.run(**basic_printer_arguments,
                             mount_dir_as_flash=flash_dir,
                             xflash_content=flash,
                             eeprom_content=(eeprom_bank_1,
                                             eeprom_bank_2)) as printer:
        try:
            await asyncio.wait_for(wait_for_bootstrap(printer), timeout=900.0)
        except asyncio.TimeoutError:
            pytest.fail('timed out while waiting for xflash bootstrap')


@pytest_asyncio.fixture
async def xflash_content(basic_printer_arguments, firmware_path, tmpdir,
                         request):
    # create empty xflash content
    xflash_size = 2**23
    xflash_path = Path(tmpdir / 'xflash.bin')
    with open(xflash_path, 'wb') as f:
        f.seek(xflash_size - 1)
        f.write(b'\x00')

    # xflash content
    requested_xflash_hash = get_hash('v1', Path(firmware_path))
    key = f'xflash-content:{requested_xflash_hash}'
    xflash_content = request.config.cache.get(key, None)
    if xflash_content:
        with open(xflash_path, 'wb') as f:
            f.write(binascii.unhexlify(xflash_content))
            return xflash_path

    await prepare_xflash_content(
        firmware_path,
        basic_printer_arguments=basic_printer_arguments,
        tmpdir=tmpdir,
        flash=xflash_path)

    # save it for later
    request.config.cache.set(key, xflash_path.read_bytes().hex())

    # and finally return
    return xflash_path


async def prepare_eeprom_content(eeprom_variables, eeprom_bank_1: Path,
                                 eeprom_bank_2: Path):
    bank_data = eeprom.generate_bank(
        sequence_id=1,
        version=1,
        items=[(name, bytes(value))
               for name, value in eeprom_variables.items()])
    eeprom_bank_1.write_bytes(bank_data)


@pytest_asyncio.fixture
async def eeprom_content(eeprom_variables, tmpdir):
    # create empty eeprom banks
    bank_size = 8192
    bank_1 = Path(tmpdir / 'eeprom_bank1.bin')
    bank_2 = Path(tmpdir / 'eeprom_bank2.bin')
    bank_paths = (bank_1, bank_2)

    for bank_path in bank_paths:
        with open(bank_path, 'wb') as f:
            f.seek(bank_size - 1)
            f.write(b'\x00')

    # empty eeprom requested
    if not eeprom_variables:
        return bank_paths

    # generate the eeprom content
    await prepare_eeprom_content(eeprom_variables, bank_1, bank_2)

    # and finally return
    return bank_paths


@pytest.fixture
def printer_flash_dir(tmpdir):
    return Path(tmpdir.mkdir('printer_flash_dir'))


@pytest.fixture
def basic_printer_arguments(simulator_path, firmware_path,
                            simulator_scriptio_port, simulator_proxy_port,
                            tmpdir, pytestconfig) -> dict:
    enable_graphic = pytestconfig.getoption('--enable-graphic')

    return dict(simulator_path=simulator_path,
                machine=MachineType.MK4,
                firmware_path=firmware_path,
                scriptio_port=simulator_scriptio_port,
                http_proxy_port=simulator_proxy_port,
                tmpdir=Path(tmpdir.mkdir('simulator')),
                nographic=not enable_graphic)


@pytest_asyncio.fixture
async def printer_factory(basic_printer_arguments, xflash_content,
                          eeprom_content, printer_flash_dir, pytestconfig):
    extra_arguments = []
    if pytestconfig.getoption('--gdb'):
        extra_arguments.append('-s')
    return functools.partial(Simulator.run,
                             **basic_printer_arguments,
                             xflash_content=xflash_content,
                             eeprom_content=eeprom_content,
                             mount_dir_as_flash=printer_flash_dir,
                             extra_arguments=extra_arguments)


@pytest_asyncio.fixture
async def printer(printer_factory):
    async with printer_factory() as printer:
        yield printer
