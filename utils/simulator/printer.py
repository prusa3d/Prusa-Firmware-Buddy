from enum import Enum, auto
from PIL import Image

try:
    from typing import Protocol
except ImportError:
    # Protocol isn't available on Python < 3.8
    class Protocol:
        pass


class MachineType(Enum):
    MINI = 'prusa-mini'


class Thermistor(Enum):
    BED = auto()
    NOZZLE = auto()
    HEATBREAK = auto()


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
