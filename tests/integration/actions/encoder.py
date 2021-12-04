from simulator import Printer


async def click(printer: Printer):
    await printer.encoder_click()


async def push(printer: Printer):
    await printer.encoder_push()


async def release(printer: Printer):
    await printer.encoder_release()


async def rotate_left(printer: Printer):
    await printer.encoder_rotate_left()


async def rotate_right(printer: Printer):
    await printer.encoder_rotate_right()
