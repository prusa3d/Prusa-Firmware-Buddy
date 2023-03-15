import ipaddress
import http_server_test_funcs as test
import argparse

#   IF YOU WANT TO ADD A TEST
#       add test data in server_tests/tests.json with JSON syntax
#       check if current test implementation will test it properly:
#           http_server_test_funcs.py > send_request()  - if JSON request, dont add content-type, it is added automatically
#                                       test_response()


def main():
    # parse script args
    parser = argparse.ArgumentParser()
    parser.add_argument("ip", help="IP address of the tested printer")
    args = parser.parse_args()

    # check if script argument is valid
    try:
        ip = ipaddress.ip_address(args.ip)
    except ValueError:
        print('Address \'%s\' is invalid' % args.ip)
        return

    test.init(args.ip)
    test.test_loop()


main()
