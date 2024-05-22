import asyncio
import logging
import functools
import io

from easyocr import Reader
from PIL import Image

from simulator import Printer, MachineType
from ..extra.timeout import timeoutable

logger = logging.getLogger(__name__)
ocr_reader = Reader(['en'], verbose=False)


async def take_screenshot(printer: Printer) -> Image.Image:
    return await printer.screen_take_screenshot()


async def read(printer: Printer):
    screenshot = await take_screenshot(printer)
    screenshot_io = io.BytesIO()
    screenshot.save(screenshot_io, format='PNG')
    boxes = await asyncio.get_event_loop().run_in_executor(
        None,
        functools.partial(ocr_reader.readtext,
                          screenshot_io.getvalue(),
                          detail=0))
    words = []
    for box in boxes:
        words += [word.strip() for word in box.split()]
    text = ' '.join(words)
    logger.info('text on screen: %s', text)
    return text


@timeoutable
async def wait_for_text(printer: Printer, text: str):
    """ Wait for the text to appear on screen.
    Text is compared lower case because the OCR won't know c/C apart."""
    while True:
        text_on_screen = await read(printer)
        if text.lower() in text_on_screen.lower():
            return text_on_screen
        await asyncio.sleep(1)


async def is_on_homescreen(printer: Printer):
    text = await read(printer)
    fragments = 'preheat', 'settings'
    return all(fragment in text.lower() for fragment in fragments)


async def is_booting(printer: Printer):
    text = await read(printer)
    if not text.strip():
        # the simulator might start with an empty/black
        return True
    if printer.machine == MachineType.MK4 and text.strip():
        # after the black screen, we might catch the loading screen
        fragments = 'loadin', 'looking for bbf', 'preparing', 'copying', 'installing'
        return any(fragment in text.lower() for fragment in fragments)
    return False
