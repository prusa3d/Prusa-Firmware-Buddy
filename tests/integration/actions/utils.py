from asyncio import sleep
import logging
from . import encoder, screen
from simulator import Printer
from ..extra.timeout import timeoutable

logger = logging.getLogger(__name__)


async def leave_preview(printer: Printer, text: str):
    """If the printer automatically shows preview screen, click return."""
    preview_fragments = {'print', 'back'}

    if all(fragment in text.lower() for fragment in preview_fragments):
        # Select back and click
        logger.info('clicking back on preview screen')
        await encoder.rotate_right(printer)
        await encoder.click(printer)

        # Wait for the preview screen to disappear
        while all(fragment in text.lower() for fragment in preview_fragments):
            await sleep(1)
            text = await screen.read(printer)

        return True

    return False


@timeoutable
async def wait_for_bootstrap(printer: Printer):
    """Wait for the printer to boot up and be ready to use.
    Click away startup screens if necessary."""

    click_language = True
    text = ''
    while not all(fragment in text.lower()
                  for fragment in {'preheat', 'settings'}):
        text = await screen.read(printer)

        # If there is language selection, click ok
        if click_language and all(fragment in text.lower()
                                  for fragment in {'select', 'language'}):
            click_language = False
            # In language select screen, click ok
            logger.info('clicking ok on language select screen')
            await encoder.click(printer)

        await leave_preview(printer, text)

        await sleep(1)

    logger.info('---------- Bootstrap done ----------')


async def enter_menu(printer: Printer, moves: int, menu_check: str = None):
    """Enter a menu by rotating the encoder and clicking it.
    When using menu_check, wait for some later item in the menu.
    Waiting for first item can loose clicks and get lost.
    Do not use the menu name. String menu_check is not case sensitive,
    so it would match the button and not wait to go into the menu."""
    ITEM_CHANGE_DELAY_S = 1

    for _ in range(moves):
        await encoder.rotate_right(printer)
        await sleep(ITEM_CHANGE_DELAY_S)
    await encoder.click(printer)

    if menu_check:
        await screen.wait_for_text(printer, menu_check)
    else:
        await sleep(1)
