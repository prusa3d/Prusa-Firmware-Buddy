from ipaddress import ip_address
import socketserver
import http.server
import argparse
import connect_test_funcs as test

#   IF YOU WANT TO ADD SOME TEST:
#       create json test structure with proper syntax in tests.json > "tests" array
#       add testing solution in test_json_body() at connect_test_funcs.py


class TestTCPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        # self.request is the TCP socket connected to the client
        self.data = self.request.recv(1024).strip()
        # remove "b'xxx'" in the string
        data_str = str(self.data)[2:-1]
        # some tests starts as a response to
        if "/p/telemetry" in data_str:
            # testing telemetry in every cycle
            test.test_telemetry(data_str)
            # returns test data or HTTP_OK depending on next_delay
            ret_data = test.generate_response()
            # send response to printer
            self.request.sendall(ret_data.encode('utf-8'))
        else:
            # start testing
            test.test_printers_response(data_str)


def main():

    parser = argparse.ArgumentParser(description='starts http server for test')
    parser.add_argument('ip_address',
                        metavar="host_ip",
                        type=ip_address,
                        help='host ip address')
    args = parser.parse_args()

    HOST = args.ip_address
    PORT = 80

    test.tests_init()

    print('IP address of server connected:' + str(HOST))
    httpd = socketserver.TCPServer((str(HOST), PORT), TestTCPHandler)

    httpd.serve_forever()


main()
