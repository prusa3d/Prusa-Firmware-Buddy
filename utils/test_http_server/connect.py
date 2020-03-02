import socketserver  # import socketserver preinstalled module
import http.server
import argparse
from ipaddress import ip_address

#HOST = '192.168.1.111'
PORT = 9000

parser = argparse.ArgumentParser(description='starts http server for test')
parser.add_argument('ip_address',
                    metavar="host_ip",
                    type=ip_address,
                    help='host ip address')
args = parser.parse_args()


class TestTCPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        # self.request is the TCP socket connected to the client
        self.data = self.request.recv(1024).strip()
        print("{} wrote:".format(self.client_address[0]))
        print(self.data)
        ret_data = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"command\":\"g28\"}"
        self.request.sendall(ret_data.encode('utf-8'))


HOST = args.ip_address
print('IP address of server connected:' + str(HOST))
# standard http server
#httpd = socketserver.TCPServer((str(HOST), PORT), http.server.SimpleHTTPRequestHandler)
#custom server
httpd = socketserver.TCPServer((str(HOST), PORT), TestTCPHandler)
httpd.serve_forever()
