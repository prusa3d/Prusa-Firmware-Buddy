import asyncio
import aiohttp
import pytest
from pathlib import Path

from .actions import encoder, screen, temperature, network
from simulator import MachineType, Thermistor, Printer

pytestmark = pytest.mark.asyncio


async def leave_preview(printer: Printer, text: str):
    """If the printer automatically shows preview screen, click return."""
    preview_fragments = {'print', 'back'}

    if all(fragment in text.lower() for fragment in preview_fragments):
        # Select back and click
        await encoder.rotate_right(printer)
        await encoder.click(printer)

        # Wait for the preview screen to disappear
        while all(fragment in text.lower() for fragment in preview_fragments):
            await asyncio.sleep(1)
            text = await screen.read(printer)

        return True

    return False


async def wait_for_bootstrap(printer: Printer):
    """Wait for the printer to boot up and be ready to use.
    Click ok on ipnut shaper info if necessary."""

    click_is_info = True
    text = ''
    while not all(fragment in text.lower()
                  for fragment in {'preheat', 'settings'}):
        text = await screen.read(printer)

        # If input_shaper branch shows info screen, click ok
        if click_is_info and all(fragment in text.lower()
                                 for fragment in {'input', 'shaper'}):
            click_is_info = False
            # In input_shaper intro screen, click ok
            await encoder.click(printer)

        await leave_preview(printer, text)

        await asyncio.sleep(1)


async def test_max_temp_error_on_bed(printer: Printer):
    await wait_for_bootstrap(printer)
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
        await wait_for_bootstrap(printer)

        # Click print, this click can get lost if the printer automatically shows preview screen now
        await encoder.click(printer)

        # Wait for the files to appear
        text = ''
        while not all(
                filename.split(' ')[0] in text for filename in filenames):
            text = await screen.read(printer)

            if await leave_preview(printer, text):
                # Click print again to show browser
                await encoder.click(printer)

            await asyncio.sleep(1)
