from asyncio import sleep
import pytest

from .actions import screen, temperature, encoder, utils
from simulator import Thermistor, Printer

pytestmark = pytest.mark.asyncio


async def _enable_heater(printer: Printer, heater: Thermistor, temp: int):
    await utils.enter_menu(printer, 3, "Disable Motors")
    await sleep(1)

    # Choose temperature
    await utils.enter_menu(printer, 2, "Print Fan Speed")
    await sleep(1)

    # Choose heater
    if heater == Thermistor.NOZZLE:
        await utils.enter_menu(printer, 1)
    elif heater == Thermistor.BED:
        await utils.enter_menu(printer, 2)
    else:
        raise NotImplementedError

    # Set target temp
    for t in range(temp):
        await encoder.rotate_right(printer)
        await sleep(0.2)
    await encoder.click(printer)


# Nozzle
async def test_min_temp_error_on_nozzle(printer: Printer):
    await utils.wait_for_bootstrap(printer)
    await _enable_heater(printer, Thermistor.NOZZLE, 30)
    await temperature.set(printer, Thermistor.NOZZLE, -10)
    await screen.wait_for_text(printer, 'MINTEMP ERROR')


# Skip this until simulator update, because simulator 0.9.9 cannot go past 300℃
@pytest.mark.skip()
async def test_max_temp_error_on_nozzle(printer: Printer):
    await utils.wait_for_bootstrap(printer)
    await temperature.set(printer, Thermistor.NOZZLE, 310)
    await screen.wait_for_text(printer, 'MAXTEMP ERROR')


async def test_temp_runaway_error_on_nozzle(printer: Printer):
    await utils.wait_for_bootstrap(printer)
    await temperature.set(printer, Thermistor.NOZZLE, 50)
    await sleep(3)
    await _enable_heater(printer, Thermistor.NOZZLE, 50)
    await sleep(3)
    await temperature.set(printer, Thermistor.NOZZLE, 10)
    await sleep(45)
    await screen.wait_for_text(printer, 'THERMAL RUNAWAY')


async def test_preheat_error_on_nozzle(printer: Printer):
    await utils.wait_for_bootstrap(printer)
    await _enable_heater(printer, Thermistor.NOZZLE, 30)
    await temperature.set(printer, Thermistor.NOZZLE, 10)
    await sleep(20)
    await screen.wait_for_text(printer, 'PREHEAT ERROR')


# Bed
async def test_min_temp_error_on_bed(printer: Printer):
    await utils.wait_for_bootstrap(printer)
    await _enable_heater(printer, Thermistor.BED, 30)
    await temperature.set(printer, Thermistor.BED, -10)
    await screen.wait_for_text(printer, 'MINTEMP ERROR')


async def test_max_temp_error_on_bed(printer: Printer):
    await utils.wait_for_bootstrap(printer)
    await temperature.set(printer, Thermistor.BED, 230)
    await screen.wait_for_text(printer, 'MAXTEMP ERROR')


async def test_temp_runaway_error_on_bed(printer: Printer):
    await utils.wait_for_bootstrap(printer)
    await temperature.set(printer, Thermistor.BED, 30)
    await sleep(3)
    await _enable_heater(printer, Thermistor.BED, 30)
    await sleep(3)
    await temperature.set(printer, Thermistor.BED, 10)
    await sleep(60)
    await screen.wait_for_text(printer, 'THERMAL RUNAWAY')


async def test_preheat_error_on_bed(printer: Printer):
    await utils.wait_for_bootstrap(printer)
    await _enable_heater(printer, Thermistor.BED, 30)
    await sleep(3)
    await temperature.set(printer, Thermistor.BED, 10)
    await sleep(20)
    await screen.wait_for_text(printer, 'PREHEAT ERROR')


# Heatbreak
async def test_min_temp_error_on_heatbreak(printer: Printer):
    await utils.wait_for_bootstrap(printer)
    await _enable_heater(printer, Thermistor.NOZZLE, 30)
    await temperature.set(printer, Thermistor.HEATBREAK, 0)
    await screen.wait_for_text(printer, 'MINTEMP ERROR')


async def test_max_temp_error_on_heatbreak(printer: Printer):
    await utils.wait_for_bootstrap(printer)
    await temperature.set(printer, Thermistor.HEATBREAK, 200)
    await screen.wait_for_text(printer, 'MAXTEMP ERROR')
