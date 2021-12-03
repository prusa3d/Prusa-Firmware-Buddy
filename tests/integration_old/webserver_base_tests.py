import http.client
import pprint
import argparse

parser = argparse.ArgumentParser(description='basic tests for mini webserver')
parser.add_argument('--ip',
                    type=str,
                    help='mini webserver ip address',
                    required=False)
parser.add_argument('-a',
                    '--all',
                    action='store_true',
                    required=False,
                    help='test all')
args = parser.parse_args()

args.ip = "192.168.1.216"

connection = http.client.HTTPConnection(args.ip)
# test basic get request
print("basic GET request")
connection.request("GET", "/")
response = connection.getresponse()
print("Status: {} and reason: {}".format(response.status, response.reason))
headers = response.getheaders()
pp = pprint.PrettyPrinter(indent=4)
pp.pprint("Headers: {}".format(headers))

print("GET request for parmeters in json structure")
connection.request("GET", "/variable")
response = connection.getresponse()
print("Status: {} and reason: {}".format(response.status, response.reason))
headers = response.getheaders()
pp = pprint.PrettyPrinter(indent=4)
pp.pprint("Headers: {}".format(headers))

# print("request to list all options")
# connection.request("OPTIONS", "")
# response = connection.getresponse()
# print("Status: {} and reason: {}".format(response.status, response.reason))
# print("Msg: {}".format(response.msg))
# headers = response.getheaders()
# pp = pprint.PrettyPrinter(indent=4)
# pp.pprint("Headers: {}".format(headers))

connection.close()
