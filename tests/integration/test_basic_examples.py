import asyncio
import pytest
from pathlib import Path

from .actions import encoder, screen, temperature, utils
from simulator import Thermistor, Printer

pytestmark = pytest.mark.asyncio


async def test_max_temp_error_on_bed(printer: Printer):
    await utils.wait_for_bootstrap(printer)
    await temperature.set(printer, Thermistor.BED, 230)
    await screen.wait_for_text(printer, 'MAXTEMP ERROR')


async def test_filebrowser_shows_files(printer_factory,
                                       printer_flash_dir: Path):
    # prepare files on the flash drive
    filenames = ['first file.gcode', 'second file.gcode']
    for filename in filenames:
        (printer_flash_dir / filename).write_bytes(b'')

    # check they appear on the screen
    async with printer_factory() as printer:
        printer: Printer
        await utils.wait_for_bootstrap(printer)

        # Click print, this click can get lost if the printer automatically shows preview screen now
        await encoder.click(printer)

        # Wait for the files to appear
        text = ''
        while not all(
                filename.split(' ')[0] in text for filename in filenames):
            text = await screen.read(printer)

            if await utils.leave_preview(printer, text):
                # Click print again to show browser
                await encoder.click(printer)

            await asyncio.sleep(1)
