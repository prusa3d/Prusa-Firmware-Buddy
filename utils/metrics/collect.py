import logging
import logging.config
import yaml
import socket
import re
import bisect
import asyncio
import click
import aioinflux
import uuid

from typing import Union, Optional
from datetime import datetime, timedelta, timezone
from line_protocol_parser import parse_line, LineFormatError

logger = logging.getLogger(__name__)
handler_identifier = uuid.uuid4().hex[:8]


class MetricError(Exception):
    def __init__(self, message):
        self.message = message
        super().__init__(message)


class Point:
    def __init__(self,
                 timestamp: Union[int, datetime],
                 metric_name,
                 value: Union[dict, MetricError, int, float, str],
                 tags: Optional[dict] = None):

        # relative if int, absolute if datetime
        self.timestamp = timestamp
        self.metric_name = metric_name
        if isinstance(value, MetricError):
            self.values = dict(error=value.message)
        elif not isinstance(value, dict):
            self.values = dict(value=value)
        else:
            self.values = value
        self.tags = tags or dict()

    @property
    def is_error(self):
        return 'error' in self.values

    def __repr__(self):
        return 'Point(%s, %s, %s)' % (self.timestamp, self.metric_name,
                                      self.values)


class LineProtocolParser:
    valid_metric_name_re = re.compile(r'^[a-zA-Z_0-9]+$')

    def __init__(self, version):
        self.version = version
        assert self.version in [2, 3, 4]

    def parse(self, text):
        for line in text.splitlines():
            try:
                data = parse_line(line)
            except LineFormatError:
                if self.version >= 4 and 'value too long' not in line:
                    logger.warning('received invalid line: %r', line)
                continue

            fields = data['fields']
            if len(fields) == 1 and 'v' in fields:
                fields = dict(value=fields['v'])

            metric_name = data['measurement']

            if self.version == 3 and '_seq' in data['tags']:
                # ignore metric_record_log metrics which were for a few weeks part of the firmware & were overflowing the database
                continue

            if LineProtocolParser.valid_metric_name_re.match(
                    metric_name) is None:
                logger.warning("Invalid metric name %r", metric_name)
                yield Point(
                    int(data['time']), "metric_error",
                    dict(error_type="parse",
                         metric_name=metric_name,
                         message=text), dict())
                continue

            if 'value' in fields and fields['value'] == float('nan'):
                logger.info('skipping NaN value of %s', metric_name)
                continue

            yield Point(int(data['time']), metric_name, fields, data['tags'])

    def make_timedelta(self, timestamp):
        if self.version < 4:
            return timedelta(milliseconds=int(timestamp))
        else:
            return timedelta(microseconds=int(timestamp))


class SyslogHandlerClient(asyncio.DatagramProtocol):
    syslog_msg_re = re.compile(
        r'<(?P<PRI>\d+)>1\s+(?P<TM>\S+)\s+(?P<HOST>\S+)'
        r'\s+(?P<APP>\S+)\s+(?P<PROCID>\S+)\s+(?P<SD>\S+)'
        r'\s+(?P<MSGID>\S+)\s+(?P<MSG>.*)', re.MULTILINE | re.DOTALL)

    parsers = {
        2: LineProtocolParser(version=2),
        3: LineProtocolParser(version=3),
        4: LineProtocolParser(version=4),
    }

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
        if version not in self.parsers:
            logging.warning('received unsupported version %s', version)
            return
        parser = self.parsers[version]
        points = list(parser.parse(serialized_points))
        msgid = int(headerdict['msg'])
        printer.register_received_message(msgid)
        self.handle_fn(
            Point(datetime.now(tz=timezone.utc), 'udp_datagram',
                  dict(size=8 + data_size, points=len(points)),
                  dict(mac_address=printer.mac_address)))
        msgdelta = parser.make_timedelta(int(headerdict['tm']))
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
                    timestamp += parser.make_timedelta(point.timestamp)
                    point.timestamp = timestamp  # type: ignore
                elif version in (3, 4):
                    # timestamp in v3 is relative to the first point in the message
                    point.timestamp = timestamp + parser.make_timedelta(  # type: ignore
                        point.timestamp)
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
                except Exception:
                    logger.exception('failure when collecting reports')
            await asyncio.sleep(1.0)

    async def run(self):
        loop = asyncio.get_event_loop()
        asyncio.ensure_future(self.create_periodic_reports())
        await loop.create_datagram_endpoint(lambda: self,
                                            local_addr=(self.host, self.port),
                                            reuse_port=True)


class Application:
    def __init__(self, influx, syslog_address):
        self.influx: aioinflux.InfluxDBClient = influx
        self.points = []
        self.syslog_handler = SyslogHandlerClient(syslog_address,
                                                  self.handle_point)
        self.points_counter = 0

    def handle_point(self, point):
        self.points.append(point)
        self.points_counter += 1

    async def task_write_points(self):
        while True:
            try:
                self.handle_point(
                    Point(datetime.now(tz=timezone.utc),
                          'datapoints_count',
                          value=dict(points=self.points_counter),
                          tags=dict(metric_handler_id=handler_identifier)))

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
            except aioinflux.client.InfluxDBWriteError as e:
                influxdb_error = e.headers.get("X-Influxdb-Error", "")
                pattern = r'^partial write: field type conflict: input field "(.+)" on measurement "(.+)" is type (\w+), already exists as type (\w+) dropped=(\d+)$'
                match = re.search(pattern, influxdb_error)
                if match:
                    field_name = match.group(1)
                    measurement_name = match.group(2)
                    new_type = match.group(3)
                    existing_type = match.group(4)
                    dropped_records = int(match.group(5))
                    self.handle_point(
                        Point(datetime.now(tz=timezone.utc),
                              'field_type_conflict',
                              value=dict(field_name=field_name,
                                         measurement_name=measurement_name,
                                         new_type=new_type,
                                         existing_type=existing_type,
                                         dropped_records=dropped_records),
                              tags=dict(metric_handler_id=handler_identifier)))
                else:
                    logging.warning('influxdb write error: %s', e)
            except Exception as e:
                logging.exception(
                    'failure when sending data points to InfluxDB')
            await asyncio.sleep(1)

    async def run(self):
        await asyncio.gather(self.task_write_points(),
                             self.syslog_handler.run())


async def main(syslog_address, database, database_password, database_username,
               database_host):
    async with aioinflux.InfluxDBClient(host=database_host,
                                        db=database,
                                        username=database_username,
                                        password=database_password) as influx:
        await influx.create_database(db=database)
        app = Application(influx, syslog_address=syslog_address)
        await app.run()


@click.command()
@click.option('--host', 'host', default='0.0.0.0')
@click.option('--port', '--syslog-port', 'port', default=8514, type=int)
@click.option('--database', default='buddy')
@click.option('--database-host', default='localhost')
@click.option('--database-username')
@click.option('--database-password')
@click.option('--logging-config', type=click.Path(dir_okay=False))
def cmd(host, port, database, database_username, database_password,
        database_host, logging_config):

    if logging_config:
        with open(logging_config, 'r') as f:
            logging.config.dictConfig(yaml.full_load(f))
    else:
        logging.basicConfig(level=logging.INFO)

    loop = asyncio.get_event_loop()
    asyncio.ensure_future(
        main(syslog_address=(host, port),
             database=database,
             database_username=database_username,
             database_password=database_password,
             database_host=database_host))
    loop.run_forever()
    loop.close()


if __name__ == '__main__':
    cmd()  # type: ignore
