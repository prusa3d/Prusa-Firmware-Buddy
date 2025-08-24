#!/usr/bin/env python3
import http.server
import ssl
from http.server import HTTPServer, BaseHTTPRequestHandler, ThreadingHTTPServer
import argparse
from ipaddress import ip_address
from io import BytesIO
import json
import time

parser = argparse.ArgumentParser(description='starts http server for test')
parser.add_argument('--ip_address',
                    metavar="host_ip",
                    type=ip_address,
                    help='host ip address')
parser.add_argument('--port', metavar="port", type=int, help='port number')

parser.add_argument('--tls',
                    default=False,
                    action='store_true',
                    help='TLS connection')

args = parser.parse_args()

if args.ip_address:
    HOST = args.ip_address
else:
    HOST = '0.0.0.0'

if args.port:
    PORT = args.port
else:
    if args.tls:
        PORT = 40000
    else:
        PORT = 8000
if args.tls:
    print('Starting http://' + str(HOST) + ':' + str(PORT))
else:
    print('Starting https://' + str(HOST) + ':' + str(PORT))

cnt = 0
test_data = None
cmd_time_delay = 0
time_mark = time.time()

with open('response/response.json', 'r') as test_content:
    test_data = json.loads(test_content.read())


class TestHTTPHandler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def do_GET(self):
        self.send_response(200)
        self.end_headers()
        self.wfile.write(b'Hello, world!')

    def do_POST(self):
        global cnt, cmd_time_delay, time_mark
        #        cnt = cnt + 1
        print(cnt + 1)
        print(self.headers)
        content_length = int(self.headers['Content-Length'])
        #content_type = self.headers['Content-Type']
        body = self.rfile.read(content_length)
        print(body)
        if ((time.time() - time_mark) > cmd_time_delay):
            time_mark = time.time()
            if self.path == "/p/telemetry":
                if test_data[cnt].get('args'):
                    ret_data = json.dumps({
                        'command': test_data[cnt]['command'],
                        'args': [test_data[cnt]['args']]
                    })
                else:
                    ret_data = json.dumps(
                        {'command': test_data[cnt]['command']})

                self.send_response(200)
                self.send_header("Connection", "keep-alive")
                self.send_header('Content-Type',
                                 test_data[cnt]['Content-Type'])
                self.send_header('Content-Length', len(ret_data))
                self.send_header('Command-Id', cnt + 1)
                self.end_headers()
                response = BytesIO()
                response.write(str.encode(ret_data))
                self.wfile.write(response.getvalue())
                cmd_time_delay = test_data[cnt]['delay_s']
                TGREEN = '\033[32m'  # Green Text
                ENDC = '\033[m'  # reset to the defaults
                print(TGREEN, ret_data, ENDC)
                cnt = cnt + 1
                if (cnt == len(test_data)):
                    cnt = 0
        else:
            self.send_response(200)


httpd = ThreadingHTTPServer((str(HOST), PORT), TestHTTPHandler)
if args.tls:
    httpd.socket = ssl.wrap_socket(
        httpd.socket,
        server_side=True,
        certfile='./tls/certificates/server-cert.pem',
        keyfile='./tls/certificates/server-key.pem',
        ssl_version=ssl.PROTOCOL_TLS)
httpd.serve_forever()
