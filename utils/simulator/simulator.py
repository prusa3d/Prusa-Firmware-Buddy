import socket
from pathlib import Path
import logging
import asyncio
import uuid
from typing import Optional, Tuple, Callable, List
from contextlib import asynccontextmanager, AsyncExitStack, closing

from PIL import Image
from bootstrap import get_dependency_directory, switch_to_venv_if_nedded
from .pubsub import Publisher
from .printer import Thermistor, MachineType, NetworkInterface

logger = logging.getLogger('simulator')

switch_to_venv_if_nedded()


class Simulator:
    def __init__(self, *, process: asyncio.subprocess.Process,
                 machine: MachineType, tmpdir: Path, logs: Publisher,
                 scriptio_reader: Optional[asyncio.StreamReader],
                 scriptio_writer: Optional[asyncio.StreamWriter],
                 http_proxy_port: Optional[int]):
        self.machine = machine
        self.process = process
        self.tmpdir = tmpdir
        self.logs = logs
        self.scriptio_reader = scriptio_reader
        self.scriptio_writer = scriptio_writer
        self.http_proxy_port = http_proxy_port

    @staticmethod
    def _get_available_port():
        with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as s:
            s.bind(('', 0))
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            return s.getsockname()[1]

    @staticmethod
    @asynccontextmanager
    async def run(
            simulator_path: Path,
            machine: MachineType,
            firmware_path: Path,
            tmpdir: Path,
            scriptio_port: Optional[int] = None,
            http_proxy_port: Optional[int] = None,
            mount_dir_as_flash: Optional[Path] = None,
            eeprom_content: Optional[Tuple[Path, Path]] = None,
            xflash_content: Optional[Path] = None,
            nographic=False,
            invoke_callback: Optional[Callable[[List[str]], None]] = None,
            extra_arguments: Optional[List[str]] = None):
        # prepare the arguments
        simulator_path = simulator_path.absolute()
        params = ['-machine', machine.value]
        params += ['-kernel', str(firmware_path.absolute())]
        # disable SOF usb interrupts, because it is not used and it makes simulator run slowly
        params += ['-global', 'STM32F4xx-usb.disable_sof_interrupt=true']
        if machine.is_puppy:
            params += ['-icount', '5']
        else:
            params += ['-icount', 'auto']
        if scriptio_port:
            params += [
                '-chardev',
                f'socket,id=p404-scriptcon,port={scriptio_port},host=localhost,server=on',
                '-global', 'p404-scriptcon.no_echo=true'
            ]
        if http_proxy_port:
            params += [
                '-netdev',
                f'user,id=mini-eth,hostfwd=tcp::{http_proxy_port}-:80'
            ]
        if mount_dir_as_flash:
            params += [
                '-drive',
                f'id=usbstick,format=raw,file=fat:rw:{mount_dir_as_flash.absolute()}'
            ]
            params += [
                '-device',
                'usb-storage,drive=usbstick',
            ]
        if eeprom_content:
            params += [
                '-drive',
                f'if=pflash,format=raw,file={str(eeprom_content[0].absolute())}'
            ]
            params += [
                '-drive',
                f'if=pflash,format=raw,file={str(eeprom_content[1].absolute())}'
            ]
        if xflash_content:
            params += [
                '-drive',
                f'if=mtd,format=raw,file={str(xflash_content.absolute())}'
            ]
        if nographic:
            params += ['-nographic']
        if extra_arguments:
            params += extra_arguments

        async with AsyncExitStack() as stack:
            # start the simulator
            logger.info('starting simulator with command: %s %s',
                        simulator_path, ' '.join(params))
            if invoke_callback:
                invoke_callback([str(simulator_path)] + params)
            process = await asyncio.create_subprocess_exec(
                str(simulator_path),
                *params,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
                cwd=firmware_path.parent)

            # connect over tcp to scriptio console
            while scriptio_port:
                if process.returncode is not None:
                    raise RuntimeError('simulator exited unexpectedly')
                try:
                    scriptio_reader, scriptio_writer = await stack.enter_async_context(
                        Simulator.connect_to_scriptio('localhost',
                                                      scriptio_port))
                    break
                except OSError:
                    logger.info('waiting for scriptio console to start')
                    await asyncio.sleep(1)
            else:
                scriptio_reader, scriptio_writer = None, None

            # start parsing stdout/logs
            logs = Publisher()

            async def parse_simulator_stdout():
                while process.stdout and not process.stdout.at_eof():
                    line = (await
                            process.stdout.readline()).decode('utf-8').strip()
                    logger.info('%s', line)
                    await logs.publish(line)

            async def parse_simulator_stderr():
                while process.stderr and not process.stderr.at_eof():
                    line = (await
                            process.stderr.readline()).decode('utf-8').strip()
                    logger.error('%s', line)

            logs_task = asyncio.ensure_future(parse_simulator_stdout())
            stderr_task = asyncio.ensure_future(parse_simulator_stderr())

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
                if process.returncode is None:
                    process.terminate()
                logs_task.cancel()
                stderr_task.cancel()
                if process.returncode is None:
                    await process.communicate()

    @staticmethod
    @asynccontextmanager
    async def connect_to_scriptio(host, port, consume_first_line=True):
        logger.info('connecting to scriptio console on %s:%d', host, port)
        scriptio_reader, scriptio_writer = await asyncio.open_connection(
            host, port)

        logger.info('connected to scriptio console on %s:%d', host, port)
        if consume_first_line:
            # even in no-echo mode, the scriptio console currently prints one line at the beginning
            # so lets read it
            await scriptio_reader.readline()

        yield scriptio_reader, scriptio_writer
        scriptio_writer.close()
        await scriptio_writer.wait_closed()

    @staticmethod
    def default_simulator_path() -> Optional[Path]:
        mini404_dep_path = get_dependency_directory('mini404')
        simulator_path = mini404_dep_path / 'qemu-system-buddy'
        if simulator_path.exists():
            return simulator_path
        else:
            return None

    def simulator_is_running(self):
        if self.process.returncode is not None:
            return False
        return True

    async def command(self, command: str, readline=False, timeout=3.0):
        assert self.simulator_is_running(), 'simulator is not running'
        assert self.scriptio_reader is not None and self.scriptio_writer is not None

        async def issue_command():
            assert self.scriptio_writer, 'scriptio socket isn\'t connected'
            self.scriptio_writer.write(command.encode('utf-8') + b'\n')

        async def wait_for_script_finished_line():
            async for line in self.logs:
                if line.strip() == 'ScriptHost: Script FINISHED':
                    break

        await asyncio.gather(
            issue_command(),
            asyncio.wait_for(wait_for_script_finished_line(), timeout=timeout))

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

    async def screen_take_screenshot(self, path=None) -> Image.Image:
        screenshot_path = path if path else self.tmpdir / (str(uuid.uuid4()) +
                                                           '.png')
        await self.command(
            f'generic-spi-display::Screenshot({screenshot_path})')
        return Image.open(screenshot_path)

    #
    # temperature primitives
    #

    def _get_thermistor_idx(self, thermistor: Thermistor):
        if self.machine == MachineType.MINI and thermistor == Thermistor.BED:
            return 1
        elif self.machine == MachineType.MINI and thermistor == Thermistor.NOZZLE:
            return 2
        elif self.machine == MachineType.MK4 and thermistor == Thermistor.NOZZLE:
            return 0
        elif self.machine == MachineType.MK4 and thermistor == Thermistor.BED:
            return 1
        elif self.machine == MachineType.MK4 and thermistor == Thermistor.HEATBREAK:
            return 2
        elif self.machine == MachineType.MK4 and thermistor == Thermistor.BOARD:
            return 3
        elif self.machine == MachineType.MK4 and thermistor == Thermistor.CASE:
            return 4

        raise NotImplementedError('dont know the index of thermistor %s on %s',
                                  thermistor, self.machine)

    async def temperature_set(self, thermistor: Thermistor,
                              temperature: float):
        thermistor_idx = self._get_thermistor_idx(thermistor)
        await self.command(
            f'thermistor{"" if thermistor_idx == 0 else thermistor_idx}::Set({temperature})'
        )

    async def temperature_get(self, thermistor: Thermistor) -> float:
        thermistor_idx = self._get_thermistor_idx(thermistor)
        temp_str = await self.command(
            f'thermistor{"" if thermistor_idx == 0 else thermistor_idx}::GetTemp()',
            readline=True)
        return float(temp_str)  # type: ignore

    #
    # network primitives
    #

    def network_proxy_http_port_get(self):
        assert self.http_proxy_port is not None
        return self.http_proxy_port
