import socket
import re
from datetime import datetime, timedelta
import aioserial
import asyncio
import click
import aioinflux


class MetricError(Exception):
    def __init__(self, message):
        self.message = message
        super().__init__(message)


class Point:
    def __init__(self, timestamp, metric_name, value, tags):
        self.timestamp = timestamp
        self.metric_name = metric_name
        self.value = value
        self.tags = tags

    @property
    def is_error(self):
        return isinstance(self.value, MetricError)

    def __repr__(self):
        return 'Point(%s, %s, %s)' % (self.timestamp, self.metric_name,
                                      self.value)


class TextProtocolParser:
    point_re = re.compile(r'\[([^:]*):([^:]*):([^:]*):([^\[]*)]')

    def parse(self, text):
        for match in re.finditer(TextProtocolParser.point_re, text):
            timediff, metric_name, flag, value = match.group(1), match.group(
                2), match.group(3), match.group(4)
            value, tags = self.parse_value(value, flag)
            yield Point(int(timediff), metric_name, value, tags)

    def parse_value(self, value, flag, tags=None):
        tags = tags or dict()

        if '&' in value:
            value, *tags_raw = value.split('&')
            tags.update({t.split('=')[0]: t.split('=')[1] for t in tags_raw})

        if flag == 's' and value.startswith('!'):
            return self.parse_value(value[2:], flag=value[1], tags=tags)
        elif flag == 'i':
            return int(value), tags
        elif flag == 'f':
            return float(value), tags
        elif flag == 's':
            return str(value), tags
        elif flag == 'e':
            return 1, tags
        elif flag == 'error':
            return MetricError(str(value)), tags
        else:
            print('invalid value:', value)
            return None, None


class SyslogHandlerClient(asyncio.DatagramProtocol):
    syslog_msg_re = re.compile(
        r'^<(?P<PRI>\d+)>1\s+(?P<TM>\S+)\s+(?P<HOST>\S+)'
        r'\s+(?P<APP>\S+)\s+(?P<PROCID>\S+)\s+(?P<SD>\S+)'
        r'\s+(?P<MSGID>\S+)\s+(?P<MSG>.*)$')

    class RemotePrinter:
        def __init__(self, mac_address):
            self.mac_address = mac_address
            self.last_received_msgid = None
            self.session_start_time = None

    def connection_made(self, transport):
        print(transport)

    def datagram_received(self, data, addr):
        match = self.syslog_msg_re.fullmatch(data.decode('utf-8'))
        if match and match.groupdict()['APP'] == 'buddy':
            groupdict = match.groupdict()
            mac_address = groupdict['HOST']
            msg = groupdict['MSG'].strip()
            header, serialized_points = msg.split(maxsplit=1)
            headerdict = dict((v.split('=') for v in header.split(',')))
            points = list(self.parser.parse(serialized_points))
            if mac_address not in self.printers:
                self.printers[mac_address] = SyslogHandlerClient.RemotePrinter(
                    mac_address)
            printer = self.printers[mac_address]
            self.process_message(printer, headerdict, points)

    def process_message(self, printer, headerdict, points):
        msgid = int(headerdict['msg'])
        msgdelta = timedelta(milliseconds=int(headerdict['tm']))
        if printer.last_received_msgid is None:
            printer.session_start_time = datetime.utcnow() - msgdelta
            printer.last_received_msgid = msgid
        elif abs(printer.last_received_msgid - msgid) < 10:
            printer.last_received_msgid = msgid
        else:
            printer.session_start_time = datetime.utcnow() - msgdelta
            printer.last_received_msgid = msgid
        timestamp = printer.session_start_time + msgdelta
        for point in points:
            timestamp += timedelta(milliseconds=point.timestamp)
            point.timestamp = timestamp
            point.tags = dict(mac_address=printer.mac_address,
                              **point.tags,
                              **self.tags)
            self.handle_fn(point)

    def __init__(self, port, handle_fn):
        self.port = port
        self.handle_fn = handle_fn
        self.tags = self.get_session_tags()
        self.parser = TextProtocolParser()
        self.printers = dict()

    def get_session_tags(self):
        return {
            'hostname': socket.gethostname(),
            'port': self.port,
            'session_start_time':
            datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S"),
        }

    async def run(self):
        loop = asyncio.get_event_loop()
        await loop.create_datagram_endpoint(lambda: self,
                                            local_addr=('0.0.0.0', self.port),
                                            reuse_port=True)


class SerialHandlerClient:
    def __init__(self, port, handle_fn):
        self.serial = aioserial.AioSerial(port, baudrate=115200)
        self.handle_fn = handle_fn
        self.tags = self.get_session_tags()
        self.line_queue = asyncio.Queue()
        self.parser = TextProtocolParser()
        self.last_point_datetime = None

    def get_session_tags(self):
        return {
            'handler': 'serial_port',
            'hostname': socket.gethostname(),
            'port': self.serial.port,
            'session_start_time':
            datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S"),
        }

    async def task_listen_on_serial(self):
        while True:
            try:
                line = (await self.serial.readline_async()).decode('ascii')
                await self.line_queue.put(line)
            except UnicodeDecodeError as e:
                print('serial: failed to decode line', str(e))

    async def task_parse_lines(self):
        while True:
            try:
                line = (await self.line_queue.get()).strip()
                for point in self.parser.parse(line):
                    if self.last_point_datetime is None:
                        point.timestamp = datetime.utcnow()
                    else:
                        point.timestamp = self.last_point_datetime + timedelta(
                            milliseconds=point.timediff)
                    point.tags = dict(**point.tags, **self.tags)
                    self.handle_fn(point)
            except Exception as e:
                print('serial: error when parsing line', str(e))

    async def run(self):
        await asyncio.gather(self.task_listen_on_serial(),
                             self.task_parse_lines())


class Application:
    def __init__(self, influx, syslog_port=None, serial_port=None):
        self.influx: aioinflux.InfluxDBClient = influx
        self.points = []
        if serial_port:
            self.uart_handler = SerialHandlerClient(serial_port,
                                                    self.handle_point)
        else:
            self.uart_handler = None
        if syslog_port:
            self.syslog_handler = SyslogHandlerClient(syslog_port,
                                                      self.handle_point)
        else:
            self.syslog_handler = None

    def handle_point(self, point):
        self.points.append(point)

    async def task_write_points(self):
        last_sent = asyncio.get_event_loop().time()
        while True:
            try:
                points, self.points = self.points, []
                influx_points = []
                for point in points:
                    if not point.is_error:
                        influx_points.append({
                            'time': point.timestamp,
                            'measurement': point.metric_name,
                            'tags': point.tags,
                            'fields': {
                                'value': point.value
                            },
                        })
                    else:
                        print('received error point', point.metric_name,
                              point.value.message)

                print('writing', len(influx_points), 'points')
                await self.influx.write(influx_points)
            except Exception as e:
                print('error when sending points: %s' % e)
            await asyncio.sleep(1)

    async def run(self):
        tasks = [self.task_write_points()]
        if self.uart_handler:
            tasks.append(self.uart_handler.run())
        if self.syslog_handler:
            tasks.append(self.syslog_handler.run())
        await asyncio.gather(*tasks)


async def main(database, serial_port, syslog_port):
    async with aioinflux.InfluxDBClient(db=database) as influx:
        await influx.create_database(db=database)
        app = Application(influx,
                          serial_port=serial_port,
                          syslog_port=syslog_port)
        await app.run()


@click.command()
@click.option('--database', default='buddy')
@click.option('--serial')
@click.option('--syslog-port', default=8514, type=int)
def cmd(database, serial, syslog_port):
    loop = asyncio.get_event_loop()
    asyncio.ensure_future(
        main(database=database, serial_port=serial, syslog_port=syslog_port))
    loop.run_forever()
    loop.close()


if __name__ == '__main__':
    cmd()
