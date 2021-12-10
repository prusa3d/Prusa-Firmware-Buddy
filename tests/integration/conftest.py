import sys
import binascii
import functools
import asyncio
import hashlib
import pytest
import json
from pathlib import Path

# add the /utils to PATH so we can use the `simulator` package
project_root = Path(__file__).resolve().parent.parent.parent
utils_dir = project_root / 'utils'
sys.path.insert(0, str(utils_dir))

from .actions import screen
from simulator import Simulator, MachineType, Printer


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
    return pytestconfig.rootpath / 'tests' / 'integration' / 'data'


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


@pytest.fixture
def eeprom_variables():
    return {
        'LANGUAGE': get_language_code('en'),  # default lang for tests
        'RUN_SELFTEST': 0,  # skip wizard
        'PL_API_KEY': "0123456789",
    }


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


async def prepare_eeprom_content(eeprom_variables, basic_printer_arguments,
                                 tmpdir, eeprom_bank_1: Path,
                                 eeprom_bank_2: Path):
    flash_dir = tmpdir.mkdir('flash_for_eeprom_init')
    with open(flash_dir / 'AUTO.GCO', 'w') as f:
        for variable_name, variable_value in eeprom_variables.items():
            f.write(f'M505 {variable_name} {variable_value}\n')

    async def wait_for_eeprom_load(printer: Printer):
        while await screen.is_booting(printer):
            pass

    async with Simulator.run(**basic_printer_arguments,
                             mount_dir_as_flash=flash_dir,
                             eeprom_content=(eeprom_bank_1,
                                             eeprom_bank_2)) as printer:
        try:
            await asyncio.wait_for(wait_for_eeprom_load(printer), timeout=10.0)
        except asyncio.TimeoutError:
            pytest.fail('timed out while waiting for eeprom setup; '
                        'please check your firmware is compiled with '
                        '-DCUSTOM_COMPILE_OPTIONS:STRING=-DAUTOSTART_GCODE=1')


@pytest.fixture
async def eeprom_content(eeprom_variables, basic_printer_arguments, tmpdir,
                         firmware_path, request):
    # create empty eeprom banks
    bank_size = 65536
    bank_1 = tmpdir / 'eeprom_bank1.bin'
    bank_2 = tmpdir / 'eeprom_bank2.bin'
    bank_paths = (bank_1, bank_2)
    for bank_path in bank_paths:
        with open(bank_path, 'wb') as f:
            f.seek(bank_size - 1)
            f.write(b'\x00')

    # empty eeprom requested
    if not eeprom_variables:
        return bank_paths

    # look into cache and recover the eeprom content if we have it already
    requested_eeprom_hash = get_hash('v1', Path(firmware_path),
                                     json.dumps(eeprom_variables))
    key = f'eeprom-content:{requested_eeprom_hash}'
    eeprom_content = request.config.cache.get(key, None)
    if eeprom_content:
        for bank_path, bank_content_hex in zip(bank_paths, eeprom_content):
            with open(bank_path, 'wb') as f:
                f.write(binascii.unhexlify(bank_content_hex))
        return bank_paths

    # generate the eeprom content
    await prepare_eeprom_content(eeprom_variables, basic_printer_arguments,
                                 tmpdir, bank_1, bank_2)

    # save it for later
    eeprom_content = []
    for bank_path in bank_paths:
        with open(bank_path, 'rb') as f:
            eeprom_content.append(f.read().hex())
    request.config.cache.set(key, eeprom_content)

    # and finally return
    return bank_paths


@pytest.fixture
def printer_flash_dir(tmpdir):
    return tmpdir.mkdir('printer_flash_dir')


@pytest.fixture
def basic_printer_arguments(simulator_path, firmware_path,
                            simulator_scriptio_port, simulator_proxy_port,
                            tmpdir, pytestconfig) -> dict:
    enable_graphic = pytestconfig.getoption('--enable-graphic')

    return dict(simulator_path=simulator_path,
                machine=MachineType.MINI,
                firmware_path=firmware_path,
                scriptio_port=simulator_scriptio_port,
                http_proxy_port=simulator_proxy_port,
                tmpdir=Path(tmpdir.mkdir('simulator')),
                nographic=not enable_graphic)


@pytest.fixture
async def printer_factory(basic_printer_arguments, eeprom_content,
                          printer_flash_dir):
    return functools.partial(Simulator.run,
                             **basic_printer_arguments,
                             eeprom_content=eeprom_content,
                             mount_dir_as_flash=printer_flash_dir)


@pytest.fixture
async def printer(printer_factory):
    async with printer_factory() as printer:
        yield printer
