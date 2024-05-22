import sys
from enum import Enum, auto
from PIL import Image
from typing import Protocol


class MachineType(Enum):
    MINI = 'prusa-mini'
    MK4 = 'prusa-mk4-027c'
    XL = 'prusa-xl-050'
    XL_DWARF_T0 = 'prusa-xl-extruder-040-0'
    XL_DWARF_T1 = 'prusa-xl-extruder-040-1'
    XL_DWARF_T2 = 'prusa-xl-extruder-040-2'
    XL_DWARF_T3 = 'prusa-xl-extruder-040-3'
    XL_DWARF_T4 = 'prusa-xl-extruder-040-4'
    XL_DWARF_T5 = 'prusa-xl-extruder-040-5'
    XL_MODULARBED = 'prusa-xl-bed-060'

    @staticmethod
    def from_string(description: str):
        if description.lower() == 'mini':
            return MachineType.MINI
        elif description.lower() == 'mk4':
            return MachineType.MK4
        elif description.lower() == 'xl':
            return MachineType.XL
        else:
            return MachineType(description)

    @property
    def is_puppy(self):
        return self in (MachineType.XL_MODULARBED, MachineType.XL_DWARF_T0,
                        MachineType.XL_DWARF_T1, MachineType.XL_DWARF_T2,
                        MachineType.XL_DWARF_T3, MachineType.XL_DWARF_T4,
                        MachineType.XL_DWARF_T5)

    @property
    def printer_name(self):
        name = self.value
        if name.startswith('prusa-'):
            name = name[6:]
        return name.upper()


class Thermistor(Enum):
    BED = auto()
    NOZZLE = auto()
    HEATBREAK = auto()
    BOARD = auto()
    CASE = auto()


class NetworkInterface(Enum):
    ETHERNET = auto()
    WIFI = auto()


class Printer(Protocol):
    machine: MachineType

    # encoder primitives
    async def encoder_click(self):
        ...

    async def encoder_push(self):
        ...

    async def encoder_release(self):
        ...

    async def encoder_rotate_left(self):
        ...

    async def encoder_rotate_right(self):
        ...

    # screen primitives

    async def screen_take_screenshot(self) -> Image.Image:
        ...

    # temperature primitives

    async def temperature_set(self, thermistor: Thermistor,
                              temperature: float):
        ...

    async def temperature_get(self, thermistor: Thermistor) -> float:
        ...

    # network primitives

    def network_proxy_http_port_get(self) -> int:
        ...
