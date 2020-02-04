import socketserver  # import socketserver preinstalled module
import http.server

class TestTCPHandler(socketserver.BaseRequestHandler):

    def handle(self):
        # self.request is the TCP socket connected to the client
        self.data = self.request.recv(1024).strip()
        print("{} wrote:".format(self.client_address[0]))
        print(self.data)
        # just send back the same data, but upper-cased
        self.request.sendall(self.data.upper())

#httpd = socketserver.TCPServer(("192.168.1.205", 9000), http.server.SimpleHTTPRequestHandler)
httpd = socketserver.TCPServer(("192.168.1.205", 9000), TestTCPHandler)
httpd.serve_forever()