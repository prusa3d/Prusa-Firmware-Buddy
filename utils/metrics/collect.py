import logging
import logging.config
import yaml
import socket
import re
import bisect
from datetime import datetime, time, timedelta, timezone
import aioserial
import asyncio
import click
import aioinflux
from line_protocol_parser import parse_line, LineFormatError
from collections import defaultdict

logger = logging.getLogger(__name__)


class MetricError(Exception):
    def __init__(self, message):
        self.message = message
        super().__init__(message)


class Point:
    def __init__(self, timestamp, metric_name, value, tags):
        self.timestamp = timestamp
        self.metric_name = metric_name
        if isinstance(value, MetricError):
            self.values = dict(error=value.message)
        elif not isinstance(value, dict):
            self.values = dict(value=value)
        else:
            self.values = value
        self.tags = tags

    @property
    def is_error(self):
        return 'error' in self.values

    def __repr__(self):
        return 'Point(%s, %s, %s)' % (self.timestamp, self.metric_name,
                                      self.values)


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
            logger.warning('received invalid value: %r', value)
            return None, None


class LineProtocolParser:
    def __init__(self, version):
        self.version = version
        assert self.version in [2, 3]

    def parse(self, text):
        for line in text.splitlines():
            try:
                data = parse_line(line)
            except LineFormatError as e:
                logger.warning('received invalid line: %r', line)
                continue
            fields = data['fields']
            if len(fields) == 1 and 'v' in fields:
                fields = dict(value=fields['v'])
            if 'value' in fields and fields['value'] == float('nan'):
                logger.info('skipping NaN value of %s', data['measurement'])
                continue
            yield Point(int(data['time']), data['measurement'], fields,
                        data['tags'])


class SyslogHandlerClient(asyncio.DatagramProtocol):
    syslog_msg_re = re.compile(
        r'<(?P<PRI>\d+)>1\s+(?P<TM>\S+)\s+(?P<HOST>\S+)'
        r'\s+(?P<APP>\S+)\s+(?P<PROCID>\S+)\s+(?P<SD>\S+)'
        r'\s+(?P<MSGID>\S+)\s+(?P<MSG>.*)', re.MULTILINE | re.DOTALL)

    class RemotePrinter:
        def __init__(self, mac_address):
            self.mac_address = mac_address
            self.last_received_msgid = None
            self.session_start_time = None
            self.msg_timestamps = []  # sorted tuples (msgid, timestamp)

        def register_received_message(self, msgid):
            entry = (msgid, datetime.now().timestamp())
            idx = bisect.bisect_right(self.msg_timestamps, entry)
            for check_index in (idx - 1, idx, idx + 1):
                if check_index >= 0 and check_index < len(
                        self.msg_timestamps
                ) and self.msg_timestamps[check_index][0] == entry[0]:
                    logger.info('detected duplicate datagram for %s',
                                self.mac_address)
                    return
            else:
                self.msg_timestamps.insert(idx, entry)

        def create_report(self, interval=3.0):
            range_end = datetime.now().timestamp()
            range_start = range_end - interval

            # drop older data
            while len(self.msg_timestamps
                      ) and self.msg_timestamps[0][1] < range_start:
                self.msg_timestamps.pop(0)

            if not self.msg_timestamps:
                return None

            lowest_msgid, highest_msgid = self.msg_timestamps[0][
                0], self.msg_timestamps[-1][0]
            expected_number_of_msgs = highest_msgid - lowest_msgid + 1
            received_number_of_msgs = len(self.msg_timestamps)

            return dict(
                expected_number_of_messages=int(expected_number_of_msgs),
                received_number_of_messages=int(received_number_of_msgs),
                interval=float(interval),
                drop_rate=1 -
                (received_number_of_msgs / expected_number_of_msgs))

    def connection_made(self, transport):
        pass

    def datagram_received(self, data, addr):
        match = self.syslog_msg_re.fullmatch(data.decode('utf-8'))
        if match and match.groupdict()['APP'] == 'buddy':
            groupdict = match.groupdict()
            mac_address = groupdict['HOST']
            msg = groupdict['MSG'].strip()
            header, serialized_points = msg.split(maxsplit=1)
            headerdict = dict((v.split('=') for v in header.split(',')))
            if mac_address not in self.printers:
                self.printers[mac_address] = SyslogHandlerClient.RemotePrinter(
                    mac_address)
            printer = self.printers[mac_address]
            self.process_message(printer,
                                 headerdict,
                                 serialized_points,
                                 data_size=len(data))

    def process_message(self, printer, headerdict, serialized_points,
                        data_size):
        version = int(headerdict.get('v', 1))
        if version < 2:
            points = list(self.parser_v1.parse(serialized_points))
        elif version == 2:
            points = list(self.parser_v2.parse(serialized_points))
        elif version == 3:
            points = list(self.parser_v3.parse(serialized_points))
        else:
            return  # unsupported version

        msgid = int(headerdict['msg'])
        printer.register_received_message(msgid)
        self.handle_fn(
            Point(datetime.now(tz=timezone.utc), 'udp_datagram',
                  dict(size=8 + data_size, points=len(points)),
                  dict(mac_address=printer.mac_address)))
        msgdelta = timedelta(milliseconds=int(headerdict['tm']))
        if printer.last_received_msgid is None:
            printer.session_start_time = datetime.now(
                tz=timezone.utc) - msgdelta
            printer.last_received_msgid = msgid
        elif abs(printer.last_received_msgid - msgid) < 10:
            printer.last_received_msgid = msgid
        else:
            printer.session_start_time = datetime.now(
                tz=timezone.utc) - msgdelta
            printer.last_received_msgid = msgid
        timestamp = printer.session_start_time + msgdelta
        for point in points:
            try:
                if version <= 2:
                    # timestamp in v2 is relative to the previous point
                    timestamp += timedelta(milliseconds=point.timestamp)
                    point.timestamp = timestamp
                elif version == 3:
                    # timestamp in v3 is relative to the first point in the message
                    point.timestamp = timestamp + timedelta(
                        milliseconds=point.timestamp)
            except OverflowError:
                logging.info(
                    'failed to increment timestamp %d by %d ms (mac address %s)',
                    timestamp, point.timestamp, printer.mac_address)
            point.tags = dict(mac_address=printer.mac_address,
                              **point.tags,
                              **self.tags)
            self.handle_fn(point)

    def __init__(self, syslog_address, handle_fn):
        self.host, self.port = syslog_address
        self.handle_fn = handle_fn
        self.tags = self.get_session_tags()
        self.parser_v1 = TextProtocolParser()
        self.parser_v2 = LineProtocolParser(version=2)
        self.parser_v3 = LineProtocolParser(version=3)
        self.printers = dict()

    def get_session_tags(self):
        return {
            'hostname':
            socket.gethostname(),
            'port':
            self.port,
            'session_start_time':
            datetime.now(tz=timezone.utc).strftime("%Y-%m-%d %H:%M:%S"),
        }

    async def create_periodic_reports(self):
        while True:
            for printer in self.printers.values():
                try:
                    report = printer.create_report()
                    if not report:
                        continue
                    self.handle_fn(
                        Point(datetime.now(tz=timezone.utc), 'udp_stats',
                              report, dict(mac_address=printer.mac_address)))
                except Exception as e:
                    logger.exception('failure when collecting reports')
            await asyncio.sleep(1.0)

    async def run(self):
        loop = asyncio.get_event_loop()
        asyncio.ensure_future(self.create_periodic_reports())
        await loop.create_datagram_endpoint(lambda: self,
                                            local_addr=(self.host, self.port),
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
                logger.exception('failure when decoding line %r', line)

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
                logger.exception('failure when parsing line %r', line)

    async def run(self):
        await asyncio.gather(self.task_listen_on_serial(),
                             self.task_parse_lines())


class Application:
    def __init__(self, influx, syslog_address=None, serial_port=None):
        self.influx: aioinflux.InfluxDBClient = influx
        self.points = []
        if serial_port:
            self.uart_handler = SerialHandlerClient(serial_port,
                                                    self.handle_point)
        else:
            self.uart_handler = None
        if syslog_address:
            self.syslog_handler = SyslogHandlerClient(syslog_address,
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
                            'fields': point.values,
                        })
                    else:
                        influx_points.append({
                            'time':
                            point.timestamp,
                            'measurement':
                            'metric_error',
                            'tags':
                            dict(metric=point.metric_name, **point.tags),
                            'fields':
                            point.values,
                        })

                logging.info('Sending %d data points to InfluxDB',
                             len(influx_points))
                await self.influx.write(influx_points)
            except Exception as e:
                logging.exception(
                    'failure when sending data points to InfluxDB')
            await asyncio.sleep(1)

    async def run(self):
        tasks = [self.task_write_points()]
        if self.uart_handler:
            tasks.append(self.uart_handler.run())
        if self.syslog_handler:
            tasks.append(self.syslog_handler.run())
        await asyncio.gather(*tasks)


async def main(syslog_address, database, serial_port, database_password,
               database_username, database_host):
    async with aioinflux.InfluxDBClient(host=database_host,
                                        db=database,
                                        username=database_username,
                                        password=database_password) as influx:
        await influx.create_database(db=database)
        app = Application(influx,
                          serial_port=serial_port,
                          syslog_address=syslog_address)
        await app.run()


@click.command()
@click.option('--host', 'host', default='0.0.0.0')
@click.option('--port', '--syslog-port', 'port', default=8514, type=int)
@click.option('--database', default='buddy')
@click.option('--database-host', default='localhost')
@click.option('--database-username')
@click.option('--database-password')
@click.option('--serial')
@click.option('--logging-config', type=click.Path(dir_okay=False))
def cmd(host, port, database, serial, database_username, database_password,
        database_host, logging_config):

    if logging_config:
        with open(logging_config, 'r') as f:
            logging.config.dictConfig(yaml.load(f, loader=yaml.FullLoader))
    else:
        logging.basicConfig(level=logging.INFO)

    loop = asyncio.get_event_loop()
    asyncio.ensure_future(
        main(syslog_address=(host, port),
             database=database,
             serial_port=serial,
             database_username=database_username,
             database_password=database_password,
             database_host=database_host))
    loop.run_forever()
    loop.close()


if __name__ == '__main__':
    cmd()
