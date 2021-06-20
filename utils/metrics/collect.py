import socket
import re
import bisect
from datetime import timezone, datetime
import time
import aioserial
import asyncio
import click
import aioinflux
from line_protocol_parser import parse_line, LineFormatError
from collections import defaultdict


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
            print('invalid value:', value)
            return None, None


class LineProtocolParser:
    def parse(self, text):
        for line in text.splitlines():
            try:
                data = parse_line(line)
            except LineFormatError as e:
                print(e, line)
                continue
            fields = data['fields']
            if len(fields) == 1 and 'v' in fields:
                fields = dict(value=fields['v'])
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
            entry = (msgid, time.time_ns())
            idx = bisect.bisect_right(self.msg_timestamps, entry)
            for check_index in (idx - 1, idx, idx + 1):
                if check_index >= 0 and check_index < len(
                        self.msg_timestamps
                ) and self.msg_timestamps[check_index][0] == entry[0]:
                    print('duplicate datagram from %s' % self.mac_address)
                    return
            else:
                self.msg_timestamps.insert(idx, entry)

        def create_report(self, interval=3.0):
            range_end = time.time_ns()
            range_start = range_end - int(interval * 1_000_000_000)

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
            if received_number_of_msgs > expected_number_of_msgs:
                print(self.msg_timestamps)

            return dict(
                expected_number_of_messages=int(expected_number_of_msgs),
                received_number_of_messages=int(received_number_of_msgs),
                interval=float(interval),
                drop_rate=1 -
                (received_number_of_msgs / expected_number_of_msgs))

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
            if mac_address not in self.printers:
                self.printers[mac_address] = SyslogHandlerClient.RemotePrinter(
                    mac_address)
            printer = self.printers[mac_address]
            self.process_message(printer, headerdict, serialized_points)

    def process_message(self, printer, headerdict, serialized_points):
        if int(headerdict.get('v', 1)) > 3:
            # unsupported version (too new)
            return
        if int(headerdict.get('v', 1)) < 2:
            points = list(self.parser_v1.parse(serialized_points))
        else:
            points = list(self.parser_v2.parse(serialized_points))
        if int(headerdict.get('v', 1)) < 3:
            timestamp_to_ns = lambda t: t * 1_000_000
        else:
            timestamp_to_ns = lambda t: t * 1_000
        msgid = int(headerdict['msg'])
        printer.register_received_message(msgid)
        msgdelta = int(headerdict['tm'])
        msgdelta = timestamp_to_ns(int(headerdict['tm']))
        if printer.last_received_msgid is None:
            printer.session_start_time = time.time_ns() - msgdelta
            printer.last_received_msgid = msgid
        elif abs(printer.last_received_msgid - msgid) < 10:
            printer.last_received_msgid = msgid
        else:
            printer.session_start_time = time.time_ns() - msgdelta
            printer.last_received_msgid = msgid
        timestamp = printer.session_start_time + msgdelta
        for point in points:
            timestamp += timestamp_to_ns(point.timestamp)
            point.timestamp = timestamp
            point.tags = dict(mac_address=printer.mac_address,
                              **point.tags,
                              **self.tags)
            self.handle_fn(point)

    def __init__(self, port, handle_fn):
        self.port = port
        self.handle_fn = handle_fn
        self.tags = self.get_session_tags()
        self.parser_v1 = TextProtocolParser()
        self.parser_v2 = LineProtocolParser()
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
                        Point(time.time_ns(), 'udp_stats', report,
                              dict(mac_address=printer.mac_address)))
                except Exception as e:
                    print('failure when collection reports: %s' % e)
            await asyncio.sleep(1.0)

    async def run(self):
        loop = asyncio.get_event_loop()
        asyncio.ensure_future(self.create_periodic_reports())
        await loop.create_datagram_endpoint(lambda: self,
                                            local_addr=('0.0.0.0', self.port),
                                            reuse_port=True)


class Application:
    def __init__(self, influx, syslog_port=None):
        self.influx: aioinflux.InfluxDBClient = influx
        self.points = []
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

                print('writing', len(influx_points), 'points')
                await self.influx.write(influx_points)
            except Exception as e:
                print('error when sending points: %s' % e)
            await asyncio.sleep(1)

    async def run(self):
        tasks = [self.task_write_points()]
        if self.syslog_handler:
            tasks.append(self.syslog_handler.run())
        await asyncio.gather(*tasks)


async def main(database, syslog_port, database_password, database_username,
               database_host):
    async with aioinflux.InfluxDBClient(host=database_host,
                                        db=database,
                                        username=database_username,
                                        password=database_password) as influx:
        await influx.create_database(db=database)
        app = Application(influx, syslog_port=syslog_port)
        await app.run()


@click.command()
@click.option('--database', default='buddy')
@click.option('--database-host', default='localhost')
@click.option('--database-username')
@click.option('--database-password')
@click.option('--syslog-port', default=8500, type=int)
def cmd(database, syslog_port, database_username, database_password,
        database_host):
    loop = asyncio.get_event_loop()
    asyncio.ensure_future(
        main(database=database,
             syslog_port=syslog_port,
             database_username=database_username,
             database_password=database_password,
             database_host=database_host))
    loop.run_forever()
    loop.close()


if __name__ == '__main__':
    cmd()
