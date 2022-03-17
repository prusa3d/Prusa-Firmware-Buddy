from pathlib import Path
import logging
import asyncio
import uuid
from typing import Optional, Tuple
from contextlib import asynccontextmanager

from PIL import Image
from bootstrap import get_dependency_directory
from .pubsub import Publisher
from .printer import Thermistor, MachineType, NetworkInterface

logger = logging.getLogger(__name__)


class Simulator:
    def __init__(self, *, process: asyncio.subprocess.Process,
                 machine: MachineType, tmpdir: Path, logs: Publisher,
                 scriptio_reader: asyncio.StreamReader,
                 scriptio_writer: asyncio.StreamWriter, http_proxy_port: int):
        self.machine = machine
        self.process = process
        self.tmpdir = tmpdir
        self.logs = logs
        self.scriptio_reader = scriptio_reader
        self.scriptio_writer = scriptio_writer
        self.http_proxy_port = http_proxy_port

    @staticmethod
    @asynccontextmanager
    async def run(simulator_path: Path,
                  machine: MachineType,
                  firmware_path: Path,
                  scriptio_port: int,
                  http_proxy_port: int,
                  tmpdir: Path,
                  mount_dir_as_flash: Path = None,
                  eeprom_content: Tuple[Path, Path] = None,
                  xflash_content: Path = None,
                  nographic=False):
        # prepare the arguments
        params = ['-machine', machine.value]
        params += ['-kernel', str(firmware_path)]
        params += [
            '-chardev',
            f'socket,id=p404-scriptcon,port={scriptio_port},host=localhost,server=on',
            '-global', 'p404-scriptcon.no_echo=true'
        ]
        params += [
            '-netdev', f'user,id=mini-eth,hostfwd=tcp::{http_proxy_port}-:80'
        ]
        if mount_dir_as_flash:
            params += [
                '-drive', f'id=usbstick,file=fat:rw:{mount_dir_as_flash}'
            ]
            params += [
                '-device',
                'usb-storage,drive=usbstick',
            ]
        if eeprom_content:
            params += ['-pflash', str(eeprom_content[0])]
            params += ['-pflash', str(eeprom_content[1])]
        if xflash_content:
            params += ['-mtdblock', str(xflash_content)]
        if nographic:
            params += ['-nographic']

        # start the simulator
        logger.info('starting simulator with command: %s %s', simulator_path,
                    ' '.join(params))
        process = await asyncio.create_subprocess_exec(
            str(simulator_path), *params, stdout=asyncio.subprocess.PIPE)

        # connect over tcp to scriptio console
        process_start_timestamp = asyncio.get_event_loop().time()
        while asyncio.get_event_loop().time() - process_start_timestamp < 10.0:
            try:
                scriptio_reader, scriptio_writer = await asyncio.open_connection(
                    'localhost', scriptio_port)
            except OSError:
                continue
            else:
                break

        # even in no-echo mode, the scriptio console currently prints one line at the beginning
        # so lets read it
        await scriptio_reader.readline()

        # start parsing stdout/logs
        logs = Publisher()

        async def parse_simulator_stdout():
            while process.stdout and not process.stdout.at_eof():
                line = (await
                        process.stdout.readline()).decode('utf-8').strip()
                logger.info('%s', line)
                await logs.publish(line)

        logs_task = asyncio.ensure_future(parse_simulator_stdout())

        # yield the simulator to the callee
        try:
            yield Simulator(process=process,
                            machine=machine,
                            tmpdir=tmpdir,
                            logs=logs,
                            scriptio_reader=scriptio_reader,
                            scriptio_writer=scriptio_writer,
                            http_proxy_port=http_proxy_port)
        finally:
            scriptio_writer.close()  # type: ignore
            await scriptio_writer.wait_closed()  # type: ignore
            process.terminate()
            logs_task.cancel()
            await process.communicate()

    @staticmethod
    def default_simulator_path() -> Optional[Path]:
        mini404_dep_path = get_dependency_directory('mini404')
        simulator_path = mini404_dep_path / 'qemu-system-buddy'
        if simulator_path.exists():
            return simulator_path
        else:
            return None

    async def command(self, command: str, readline=False):
        async def issue_command():
            assert self.scriptio_writer, 'scriptio socket isn\'t connected'
            self.scriptio_writer.write(command.encode('utf-8') + b'\n')

        async def wait_for_script_finished_line():
            async for line in self.logs:
                if line.strip() == 'ScriptHost: Script FINISHED':
                    break

        await asyncio.gather(issue_command(), wait_for_script_finished_line())

        if readline:
            line = await self.scriptio_reader.readline()
            return line.decode('utf-8').strip()

    #
    # encoder primitives
    #

    async def encoder_click(self):
        await self.command('encoder-input::Push()')

    async def encoder_push(self):
        raise NotImplementedError()

    async def encoder_release(self):
        raise NotImplementedError()

    async def encoder_rotate_left(self):
        await self.command(f'encoder-input::Twist(1)')

    async def encoder_rotate_right(self):
        await self.command(f'encoder-input::Twist(-1)')

    #
    # screen primitives
    #

    async def screen_take_screenshot(self) -> Image.Image:
        screenshot_path = self.tmpdir / (str(uuid.uuid4()) + '.png')
        await self.command(f'st7789v::Screenshot({screenshot_path})')
        return Image.open(screenshot_path)

    #
    # temperature primitives
    #

    def _get_thermistor_idx(self, thermistor: Thermistor):
        if self.machine == MachineType.MINI and thermistor == Thermistor.BED:
            return 1
        elif self.machine == MachineType.MINI and thermistor == Thermistor.NOZZLE:
            return 2
        raise NotImplementedError('dont know the index of thermistor %s on %s',
                                  thermistor, self.machine)

    async def temperature_set(self, thermistor: Thermistor,
                              temperature: float):
        thermistor_idx = self._get_thermistor_idx(thermistor)
        await self.command(f'thermistor{thermistor_idx}::Set({temperature})')

    async def temperature_get(self, thermistor: Thermistor) -> float:
        thermistor_idx = self._get_thermistor_idx(thermistor)
        temp_str = await self.command(f'thermistor{thermistor_idx}::GetTemp()',
                                      readline=True)
        return float(temp_str)  # type: ignore

    #
    # network primitives
    #

    def network_proxy_http_port_get(self):
        return self.http_proxy_port
