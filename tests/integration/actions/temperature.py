from simulator import Printer, Thermistor


async def set(printer: Printer, thermistor: Thermistor, temperature: float):
    await printer.temperature_set(thermistor, temperature)


async def get(printer: Printer, thermistor: Thermistor) -> float:
    return await printer.temperature_get(thermistor)
