import requests
import logging
import json
from datetime import datetime
import time

# global variables
IP_ADDR = ""
test_cnt = 0
tests_off = 0
test_curr = 0
json_tests = []


def init(ip_addr):
    global json_tests, test_cnt, IP_ADDR

    # set ip address
    IP_ADDR = 'http://' + ip_addr

    # load JSON tests
    json_file = open("server_tests/tests.json", "r")
    json_obj = json.load(json_file)
    json_file.close()

    json_tests = json_obj['tests']
    disabled_tests = []

    # disable switched off tests
    for test in json_tests:
        if "switch" in test:
            if "off" in test["switch"]:
                disabled_tests.append(test)

    for test in disabled_tests:
        json_tests.remove(test)

    if len(json_tests) == 0:
        print("All tests switched off")

    test_cnt = len(json_tests)

    # logging init
    logging.basicConfig(filename='server_tests/errors.log',
                        filemode='w',
                        level=logging.ERROR)


# infinite testing loop
# if all tests in tests.json are switched off, it tests only telemetry
def test_loop():
    global test_cnt

    while 1:
        test_get_telemetry()
        if test_cnt != 0:
            test()

        time.sleep(1)


def test():
    global json_tests, test_curr

    # send request according to test request data
    result_dic = send_request()
    if len(result_dic) != 0:
        # test response
        test_response(result_dic)

    # set up next test
    if (test_curr + 1) >= test_cnt:
        test_curr = 0
        off_test_cnt = 0
    else:
        test_curr += 1


def send_request():
    global test_curr, test_cnt, json_tests, IP_ADDR

    test = json_tests[test_curr]
    # parse name of the current test
    name = test['name']

    headers = {}
    res_dic = {}
    test_header = test['request']['header']
    # parse url
    ip_addr = IP_ADDR + test_header['uri']

    # load token to header if we require it in test request data
    if 'token' in test_header:
        headers.update({"Printer-Token": str(test_header['token'])})

    # send GET request
    if 'GET' in test_header['method']:
        if len(headers) != 0:
            response = requests.get(url=ip_addr,
                                    headers=headers,
                                    verify=False,
                                    timeout=2)
        else:
            response = requests.get(url=ip_addr, verify=False, timeout=2)
    # send POST request
    elif 'POST' in test_header['method']:
        if len(headers) != 0:
            response = requests.post(url=ip_addr,
                                     headers=headers,
                                     json=test['request']['body'],
                                     verify=False,
                                     timeout=2)
        else:
            response = requests.post(url=ip_addr,
                                     json=test['request']['body'],
                                     verify=False,
                                     timeout=2)
    else:
        test_failed(str(test), name + " has unsupported test method...")
        return {}

    if len(response.text) == 0:
        return {}
    # every response should be in JSON structure
    try:
        res_dic = response.json()
    except ValueError:
        test_failed(str(res_dic), "Response parsing to JSON")
        return {}

    return res_dic


# testing printer's response to request
def test_response(result_dic):
    # no responses known yet
    pass


# testing telemetry for local page
def test_get_telemetry():
    response = requests.get(IP_ADDR + "/api/telemetry")
    try:
        res_dic = response.json()
    except ValueError:
        test_failed(str(res_dic), "Telemetry parsing to JSON")
        return

    test_telemetry_response(res_dic)


# testing response from printer to local page
def test_telemetry_response(response_dic):

    if 'temp_nozzle' not in response_dic or not isinstance(
            response_dic['temp_nozzle'], int):
        test_failed(str(response_dic), "temp_nozzel in Telemetry")
    if 'temp_bed' not in response_dic or not isinstance(
            response_dic['temp_bed'], int):
        test_failed(str(response_dic), "temp_bed in Telemetry")
    if 'material' not in response_dic or not isinstance(
            response_dic['material'], str):
        test_failed(str(response_dic), "material in Telemetry")
    if 'pos_z_mm' not in response_dic or not isinstance(
            response_dic['pos_z_mm'], float):
        test_failed(str(response_dic), "pos_z_mm in Telemetry")
    if 'printing_speed' not in response_dic or not isinstance(
            response_dic['printing_speed'], int):
        test_failed(str(response_dic), "printing_speed in Telemetry")
    if 'flow_factor' not in response_dic or not isinstance(
            response_dic['flow_factor'], int):
        test_failed(str(response_dic), "flow_factor in Telemetry")


# if test fails it logs the info in error output file "connect_tests_results.txt"
def test_failed(data, name):
    now = datetime.now()
    logging.error(str(now) + " :: Test " + name + " failed:\n" + data + "\n")
