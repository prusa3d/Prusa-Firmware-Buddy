import aiohttp
import pytest

from .actions import encoder, screen, temperature, network
from simulator import MachineType, Thermistor, Printer

pytestmark = pytest.mark.asyncio


async def test_max_temp_error_on_bed(printer: Printer):
    await screen.wait_for_text(printer, 'HOME')
    await temperature.set(printer, Thermistor.BED, 230)
    await screen.wait_for_text(printer, 'MAXTEMP ERROR')


async def test_filebrowser_shows_files(printer_factory, printer_flash_dir):
    # prepare files on the flash drive
    filenames = ['first file.gcode', 'second file.gcode']
    for filename in filenames:
        (printer_flash_dir / filename).write(b'')

    # check they appear on the screen
    async with printer_factory() as printer:
        printer: Printer
        await screen.wait_for_text(printer, 'HOME')
        await encoder.click(printer)
        for filename in filenames:
            await screen.wait_for_text(printer, filename.split(' ')[0])
