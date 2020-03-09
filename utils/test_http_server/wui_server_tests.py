import subprocess
import time
import sys

args_count = len(sys.argv)
args_list = sys.argv
test_valid = 0
my_printer_ip = ''

if args_count > 1:
	my_printer_ip = args_list[1]
	test_valid = 1
else:
	print("\nRun again and add argument IP of tested printer\nexample: python ethtests.py 10.24.230.10\n")

if test_valid == 1:
	gcode_autohome = '{"command":"G28"}'
	gcode_moving = '{"command":"G1 X50 Y10 Z120", "command":"G0 X150 Y100 Z20", "command":"G1 X50 Y10 Z120", "command":"G1 X10 Y170 Z10", "command":"G1 X150 Y10 Z120"}'
	gcode_name_mistake = '{"comman":"G28"}'
	gcode_code_mistake = '{"command":"Q28"}'

	admin_valid = '{"connect_ip":"10.24.230.10","connect_key":"01234567890123456789","connect_name":"P3D"}'
	admin_not_valid_ip = '{"connect_ip":"1000.24.230.10","connect_key":"11234567890123456789","connect_name":"PRUSA3D"}'
	admin_longer_str = '{"connect_ip":"10.24.230.10","connect_key":"11114567890123456789123456","connect_name":"PRUSA3D01234567890123456789"}'

	post_header = "'Content-type: application/json'"

	TELEMETRY = my_printer_ip + '/api/telemetry'
	POST_GCODE = my_printer_ip + '/api/g-code'
	ADMIN = my_printer_ip + '/admin.html'

	for x in range(3):
		subprocess.call(["curl", "-i", "-X", "GET", TELEMETRY])
		time.sleep(2)

	print('AUTOHOME\n')
	input("\nPress Enter to continue...\n")
	subprocess.call(["curl", "-i", "-X", "POST", "-H", post_header, "--data", gcode_autohome, POST_GCODE], stdout=subprocess.DEVNULL)
	input("Press Enter to continue...\n")

	print('3 MOVING GCODES\n')
	input("Press Enter to continue...\n")
	subprocess.call(["curl", "-i", "-X", "POST", "-H", post_header, "--data", gcode_moving, POST_GCODE], stdout=subprocess.DEVNULL)

	print("GCODE mistakes begin\n")
	input("Press Enter to continue...\n")
	subprocess.call(["curl", "-i", "-X", "POST", "-H", post_header, "--data", gcode_name_mistake, POST_GCODE], stdout=subprocess.DEVNULL)

	print("GCODE mistakes end\n")
	input("Press Enter to continue...\n")
	subprocess.call(["curl", "-i", "-X", "POST", "-H", post_header, "--data", gcode_code_mistake, POST_GCODE], stdout=subprocess.DEVNULL)

	print("AUTOHOME\n")
	input("Press Enter to continue...\n")
	subprocess.call(["curl", "-i", "-X", "POST", "-H", post_header, "--data", gcode_autohome, POST_GCODE], stdout=subprocess.DEVNULL)

	print('\n\nSave .ini file and look if saved data are valid\nTest data: {"connect_ip":"10.24.230.10","connect_key":"01234567890123456789","connect_name":"P3D"}\n')
	input("Press Enter to continue...\n")
	subprocess.call(["curl", "-i", "-X", "POST", "-H", post_header, "--data", admin_valid, ADMIN], stdout=subprocess.DEVNULL)

	print('\n\nSave .ini file and look if saved data are valid\nTest data: {"connect_ip":"1000.24.230.10","connect_key":"11234567890123456789","connect_name":"PRUSA3D"}\n')
	input("Press Enter to continue...\n")
	subprocess.call(["curl", "-i", "-X", "POST", "-H", post_header, "--data", admin_not_valid_ip, ADMIN], stdout=subprocess.DEVNULL)
	
	print('\n\nSave .ini file and look if saved data are valid\nTest data: {"connect_ip":"10.24.230.10","connect_key":"11114567890123456789123456","connect_name":"PRUSA3D01234567890123456789"}\n')
	subprocess.call(["curl", "-i", "-X", "POST", "-H", post_header, "--data", admin_longer_str, ADMIN], stdout=subprocess.DEVNULL)
	
