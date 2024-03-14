#include "tester.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <cassert>
#include <cctype>
#include <algorithm>
#include <optional>
using namespace std::string_literals;

#define LIGHTMODBUS_FULL
#define LIGHTMODBUS_DEBUG
#ifndef COVERAGE_TEST
#define LIGHTMODBUS_IMPL
#endif
#include <lightmodbus/lightmodbus.h>

#define TERM_RESET   "\x1b[0m"
#define TERM_RED     "\x1b[1m\x1b[31m"
#define TERM_GREEN   "\x1b[1m\x1b[32m"
#define TERM_BLUE    "\x1b[34m"
#define TERM_MAGENTA "\x1b[1m\x1b[35m"

enum modbus_flavor
{
	MODBUS_PDU,
	MODBUS_RTU,
	MODBUS_TCP
};

struct modbus_exception_info
{
	uint8_t address;
	uint8_t function;
	ModbusExceptionCode code;
};

extern void test_main();

std::vector<uint16_t> regs(8, 0);
std::vector<uint8_t> coils(8, 0);
std::vector<uint8_t> read_locks(65536, 0);
std::vector<uint8_t> write_locks(65536, 0);
std::vector<ModbusDataCallbackArgs> received_data;
std::vector<ModbusRegisterCallbackArgs> reg_queries;
std::vector<uint8_t> request_data;
std::vector<uint8_t> response_data;
modbus_flavor modbus_mode = MODBUS_PDU;
ModbusMaster master;
ModbusErrorInfo master_error;
ModbusSlave slave;
ModbusErrorInfo slave_error;
std::optional<modbus_exception_info> slave_exception;
std::optional<modbus_exception_info> master_exception;
int transaction_id = 0;

ModbusError registerCallback(
	const ModbusSlave *slave,
	const ModbusRegisterCallbackArgs *args,
	ModbusRegisterCallbackResult *result)
{
	reg_queries.push_back(*args);

	switch (args->query)
	{
		case MODBUS_REGQ_R_CHECK:
			result->exceptionCode = read_locks[args->index] ? MODBUS_EXCEP_SLAVE_FAILURE : MODBUS_EXCEP_NONE;
			break;

		case MODBUS_REGQ_W_CHECK:
			result->exceptionCode = write_locks[args->index] ? MODBUS_EXCEP_SLAVE_FAILURE : MODBUS_EXCEP_NONE;
			break;

		case MODBUS_REGQ_R:
			switch (args->type)
			{
				case MODBUS_INPUT_REGISTER:
				case MODBUS_HOLDING_REGISTER:
					result->value = regs.at(args->index);
					break;

				case MODBUS_COIL:
				case MODBUS_DISCRETE_INPUT:
					result->value = coils.at(args->index);
					break;
			};
			break;
		
		case MODBUS_REGQ_W:
			switch (args->type)
			{
				case MODBUS_HOLDING_REGISTER:
					regs.at(args->index) = args->value;
					break;

				case MODBUS_COIL:
					coils.at(args->index) = args->value;
					break;

				default:
					throw std::runtime_error{"invalid write query!!!"};
					break;
			};
			break;
	}

	// Always return MODBUS_OK
	return MODBUS_OK;
}

ModbusError slaveExceptionCallback(const ModbusSlave *slave, uint8_t function, ModbusExceptionCode code)
{
	// printf("Slave %d exception %s (function %d)\n", address, modbusExceptionCodeStr(code), function);
	
	slave_exception = modbus_exception_info{1, function, code};

	// Always return MODBUS_OK
	return MODBUS_OK;
}

ModbusError dataCallback(const ModbusMaster *master, const ModbusDataCallbackArgs *args)
{
	received_data.push_back(*args);

	// Always return MODBUS_OK
	return MODBUS_OK;
}

ModbusError masterExceptionCallback(const ModbusMaster *master, uint8_t address, uint8_t function, ModbusExceptionCode code)
{
	// printf("Received slave %d exception %s (function %d)\n", address, modbusExceptionCodeStr(code), function);

	master_exception = modbus_exception_info{address, function, code};

	// Always return MODBUS_OK
	return MODBUS_OK;
}

template <typename T>
std::string serialize_data(const T *data, int length)
{
	std::stringstream ss;
	ss << "[";

	for (int i = 0; i < length; i++)
	{
		ss << "0x" << std::hex << std::setfill('0') << std::fixed << std::setw(sizeof(T) * 2) << static_cast<int>(data[i]);
		if (i != length - 1)
			ss << ", ";
	}
	ss << "]";
	return ss.str();
}

template <typename T>
std::string serialize_data(const std::vector<T> &vec)
{
	return serialize_data(vec.data(), static_cast<int>(vec.size()));
}

bool operator==(const ModbusErrorInfo &lhs, const ModbusErrorInfo &rhs)
{
	return modbusGetErrorCode(lhs) == modbusGetErrorCode(rhs) &&
		modbusGetErrorSource(lhs) == modbusGetErrorSource(rhs);
}

bool operator!=(const ModbusErrorInfo &lhs, const ModbusErrorInfo &rhs)
{
	return !(lhs == rhs);
}


std::string modbus_exception_str(std::optional<modbus_exception_info> ex)
{
	if (!ex.has_value()) return "none";
	std::stringstream ss;
	ss << modbusExceptionCodeStr(ex->code) << " (slave: " << (int)ex->address << ", fun: " << (int)ex->function << ")";
	return ss.str();
}

std::string error_info_str(ModbusErrorInfo info)
{
	if (modbusIsOk(info))
		return "OK";
	else
	{
		return std::string{modbusErrorSourceStr(modbusGetErrorSource(info))} 
			+ "("
			+ modbusErrorStr(modbusGetErrorCode(info))
			+ ")";
	}
}

void build_request(const std::vector<int> &args)
{
	if (args.size() < 2)
		throw std::runtime_error{"build - invalid address & function"};
	int address = args.at(0);
	int fun = args.at(1);


	request_data.clear();

	switch (modbus_mode)
	{
		case MODBUS_PDU: master_error = modbusBeginRequestPDU(&master); break;
		case MODBUS_RTU: master_error = modbusBeginRequestRTU(&master); break;
		case MODBUS_TCP: master_error = modbusBeginRequestTCP(&master); break;
	}

	if (!modbusIsOk(master_error))
		return;

	switch (fun)
	{
		case 1:
			if (args.size() < 4) throw std::runtime_error{"invalid build args"};
			master_error = modbusBuildRequest01(&master, args[2], args[3]);
			break;

		case 2:
			if (args.size() < 4) throw std::runtime_error{"invalid build args"};
			master_error = modbusBuildRequest02(&master, args[2], args[3]);
			break;

		case 3:
			if (args.size() < 4) throw std::runtime_error{"invalid build args"};
			master_error = modbusBuildRequest03(&master, args[2], args[3]);
			break;

		case 4:
			if (args.size() < 4) throw std::runtime_error{"invalid build args"};
			master_error = modbusBuildRequest04(&master, args[2], args[3]);
			break;

		case 5:
			if (args.size() < 4) throw std::runtime_error{"invalid build args"};
			master_error = modbusBuildRequest05(&master, args[2], args[3]);
			break;

		case 6:
			if (args.size() < 4) throw std::runtime_error{"invalid build args"};
			master_error = modbusBuildRequest06(&master, args[2], args[3]);
			break;

		case 15:
		{
			if (args.size() < 4) throw std::runtime_error{"invalid build args"};
			int index = args.at(2);
			int count = args.at(3);
			std::vector<uint8_t> data((count + 7) / 8);
			for (int i = 0; i < count; i++)
				modbusMaskWrite(data.data(), i, args.at(4 + i));
			master_error = modbusBuildRequest15(&master, index, count, data.data());
			break;
		}

		case 16:
		{
			if (args.size() < 4) throw std::runtime_error{"invalid build args"};
			int index = args.at(2);
			int count = args.at(3);
			std::vector<uint16_t> data(count);
			for (int i = 0; i < count; i++)
				data.at(i) = args.at(4 + i);
			master_error = modbusBuildRequest16(&master, index, count, data.data());
			break;
		}

		case 22:
		{
			if (args.size() < 5) throw std::runtime_error{"invalid build args"};
			master_error = modbusBuildRequest22(&master, args[2], args[3], args[4]);
			break;
		}

		default:
			throw std::runtime_error{"building failed - bad function"};
			break;

	}

	if (!modbusIsOk(master_error))
		return;

	switch (modbus_mode)
	{
		case MODBUS_PDU: master_error = modbusEndRequestPDU(&master); break;
		case MODBUS_RTU: master_error = modbusEndRequestRTU(&master, address); break;
		case MODBUS_TCP: master_error = modbusEndRequestTCP(&master, transaction_id++, address); break;
	}

	if (!modbusIsOk(master_error))
		return;

	const uint8_t *ptr = modbusMasterGetRequest(&master);
	int size = modbusMasterGetRequestLength(&master);
	request_data = std::vector<uint8_t>(ptr, ptr + size);
	modbusMasterFreeRequest(&master);
}

void build_exception(uint8_t address, uint8_t function, ModbusExceptionCode code)
{
	switch (modbus_mode)
	{
		case MODBUS_PDU: slave_error = modbusBuildExceptionPDU(&slave, function, code); break;
		case MODBUS_RTU: slave_error = modbusBuildExceptionRTU(&slave, address, function, code); break;
		case MODBUS_TCP: slave_error = modbusBuildExceptionTCP(&slave, transaction_id - 1, address, function, code); break;
	}

	if (!modbusIsOk(slave_error))
		return;

	const uint8_t *ptr = modbusSlaveGetResponse(&slave);
	int size = modbusSlaveGetResponseLength(&slave);
	response_data = std::vector<uint8_t>(ptr, ptr + size);
	modbusSlaveFreeResponse(&slave);
}

void parse_request()
{
	std::cout << "Parsing request..." << std::endl;

	response_data.clear();
	reg_queries.clear();
	slave_exception.reset();

	switch (modbus_mode)
	{
		case MODBUS_PDU:
			slave_error = modbusParseRequestPDU(
				&slave,
				request_data.data(),
				request_data.size());
			break;

		case MODBUS_RTU:
			slave_error = modbusParseRequestRTU(
				&slave,
				1,
				request_data.data(),
				request_data.size());
			break;

		case MODBUS_TCP:
			slave_error = modbusParseRequestTCP(
				&slave,
				request_data.data(),
				request_data.size());
			break;
	}

	if (!modbusIsOk(slave_error))
		return;

	const uint8_t *ptr = modbusSlaveGetResponse(&slave);
	int size = modbusSlaveGetResponseLength(&slave);
	response_data = std::vector<uint8_t>(ptr, ptr + size);
	modbusSlaveFreeResponse(&slave);
}

void parse_response()
{
	std::cout << "Parsing response..." << std::endl;

	received_data.clear();
	master_exception.reset();

	switch (modbus_mode)
	{
		case MODBUS_PDU:
			master_error = modbusParseResponsePDU(
				&master,
				1,
				request_data.data(),
				request_data.size(),
				response_data.data(),
				response_data.size());
			break;

		case MODBUS_RTU:
			master_error = modbusParseResponseRTU(
				&master,
				request_data.data(),
				request_data.size(),
				response_data.data(),
				response_data.size());
			break;

		case MODBUS_TCP:
			master_error = modbusParseResponseTCP(
				&master,
				request_data.data(),
				request_data.size(),
				response_data.data(),
				response_data.size());
			break;
	}

	if (received_data.size())
	{
		ModbusDataType t = received_data[0].type;
		int addr = received_data[0].address;

		if (!std::all_of(received_data.begin(), received_data.end(), [addr, t](auto &d){
			return addr == d.address && t == d.type;
		}))
			throw std::runtime_error{"incoherent received data!"};
	}
}

void dump_request()
{
	std::cout << "Request: " << serialize_data(request_data) << std::endl;
}

void dump_response()
{
	std::cout << "Response: " << serialize_data(response_data) << std::endl;
}

void dump_data()
{
	if (received_data.empty())
	{
		std::cout << "No data received" << std::endl;
	}
	else
	{
		std::cout << 
			"Received data:\n" 
			"\t    type: " << modbusDataTypeStr(received_data[0].type) << "\n"
			"\tfunction: " << static_cast<int>(received_data[0].function) << "\n";

		for (auto i = 0u; i < received_data.size(); i++)
			std::cout << "\t[" << received_data[i].index << "] = " << received_data[i].value << "\n";
	}
}

void dump_queries()
{
	std::cout << "Register queries: \n";
	for (const auto &q : reg_queries)
	{
		std::cout << "\t id: " << q.index <<
			", query: " << modbusRegisterQueryStr(q.query) <<
			", value: " << q.value << std::endl;
	}
}

void dump_master()
{
	std::cout << 
		"Master error: " << error_info_str(master_error) << "\n"
		"Master    ex: " << modbus_exception_str(master_exception) << std::endl;
}

void dump_slave()
{
	std::cout << 
		"Slave error: " << error_info_str(slave_error) << "\n"
		"Slave    ex: " << modbus_exception_str(slave_exception) << std::endl;
}

void dump_regs()
{
	std::cout << "Registers: " << serialize_data(regs) << std::endl;
}

void dump_coils()
{
	std::cout << "Coils: " << serialize_data(coils) << std::endl;
}

void assert_message(const std::string &message)
{
	std::cout << TERM_BLUE << "@ Asserting " << message << "..." TERM_RESET << std::endl;
}

void assert_master_err(ModbusErrorInfo err)
{
	assert_message("master "s + (modbusIsOk(err) ? "ok" : error_info_str(err)));

	if (master_error != err)
		throw std::runtime_error{"master status assertion failed! - got "s + error_info_str(master_error)};
}

void assert_master_ok()
{
	assert_master_err(MODBUS_NO_ERROR());
}

void assert_slave_err(ModbusErrorInfo err)
{
	assert_message("slave "s + (modbusIsOk(err) ? "ok" : error_info_str(err)));

	if (slave_error != err)
		throw std::runtime_error{"slave status assertion failed! - got "s + error_info_str(slave_error)};
}

void assert_slave_ok()
{
	assert_slave_err(MODBUS_NO_ERROR());
}

void assert_reg(int index, int value)
{
	assert_message("reg["s + std::to_string(index) + "] = " + std::to_string(value));

	if (regs.at(index) != value)
		throw std::runtime_error{"assert_reg failed"};
}

void assert_coil(int index, int value)
{
	assert_message("coil["s + std::to_string(index) + "] = " + std::to_string(value));

	if (coils.at(index) != value)
		throw std::runtime_error{"assert_coil failed"};
}

void assert_ex(
	ModbusExceptionCode ex,
	const std::optional<modbus_exception_info> &info,
	bool no_frame)
{
	if (ex == MODBUS_EXCEP_NONE && info.has_value() && !no_frame)
		throw std::runtime_error{"assertion failed - got "s + modbusExceptionCodeStr(info->code)};
	else if (ex != MODBUS_EXCEP_NONE && info.has_value() && no_frame)
	{
		if (ex != slave_exception->code)
			throw std::runtime_error{"assertion failed - got "s + modbusExceptionCodeStr(info->code)};
	}
}

void assert_slave_ex(ModbusExceptionCode ex)
{
	if (ex == MODBUS_EXCEP_NONE)
		assert_message("no slave exception");
	else
		assert_message("slave exception "s + modbusExceptionCodeStr(ex));

	assert_ex(ex, slave_exception, response_data.empty());
}

void assert_master_ex(ModbusExceptionCode ex)
{
	if (ex == MODBUS_EXCEP_NONE)
		assert_message("no master exception");
	else
		assert_message("master exception "s + modbusExceptionCodeStr(ex));

	assert_ex(ex, master_exception, response_data.empty());
}

void assert_expr(const std::string &message, bool expr)
{
	assert_message(message);
	if (!expr)
		throw std::runtime_error{"assertion failed: "s + message};
}

void set_mode(const std::string &m)
{
	if (m == "pdu")
		modbus_mode = MODBUS_PDU;
	else if (m == "rtu")
		modbus_mode = MODBUS_RTU;
	else if (m == "tcp")
		modbus_mode = MODBUS_TCP;
	else
		throw std::runtime_error{"invalid Modbus mode "s + m};
}

void set_request(const std::vector<int> &data)
{
	request_data.resize(data.size());
	std::copy(data.begin(), data.end(), request_data.begin());
}

void set_response(const std::vector<int> &data)
{
	response_data.resize(data.size());
	std::copy(data.begin(), data.end(), response_data.begin());
}

void set_reg_count(int n)
{
	if (n < 0 || n > 65536)
		throw std::runtime_error{"invalid set_reg_count()"};
	regs.resize(n);
}

void set_coil_count(int n)
{
	if (n < 0 || n > 65536)
		throw std::runtime_error{"invalid set_coil_count()"};
	coils.resize(n);
}

void clear_regs(int val)
{
	for (auto &r : regs)
		r = val;
}

void clear_coils(int val)
{
	for (auto &r : coils)
		r = val;
}

void set_rlock(int index, int lock)
{
	read_locks.at(index) = lock != 0;
}

void set_wlock(int index, int lock)
{
	write_locks.at(index) = lock != 0;
}

void reset()
{
	set_coil_count(65536);
	set_reg_count(65536);

	for (auto &v : read_locks)
		v = 0;

	for (auto &v : write_locks)
		v = 0;

	clear_coils(0);
	clear_regs(0);

	slave_exception.reset();
	master_exception.reset();
	slave_error = MODBUS_NO_ERROR();
	master_error = MODBUS_NO_ERROR();
	received_data.clear();
	reg_queries.clear();
	request_data.clear();
	response_data.clear();
}

std::string trim_leading_whitespace(std::string s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](char c) {
		return !std::isspace(c);
	}));
	return s;
}

std::string to_lowercase(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](char c) {
		return std::tolower(c);
	});
	return s;
}

void test_info(const std::string &name)
{
	std::cout << TERM_MAGENTA "==================== Test: " << name << TERM_RESET << std::endl;
}

void run_test(const std::string &name, std::function<void()> f)
{
	reset();
	test_info(name);

	try
	{
		f();
	}
	catch (const std::exception &ex)
	{
		std::cout << TERM_RED "-------- Test failed: " << ex.what() << "\n" TERM_RESET << std::endl;
		std::exit(1);
	}
	catch (...)
	{
		std::cout << TERM_RED "-------- Test failed: reason unknown!\n" TERM_RED << std::endl;
		std::exit(1);
	}

	std::cout << TERM_GREEN "-------- Test passed!\n" TERM_RESET << std::endl;
}

int main(int argc, char *argv[])
{
	slave_error = modbusSlaveInit(
		&slave,
		registerCallback,
		slaveExceptionCallback,
		modbusDefaultAllocator,
		modbusSlaveDefaultFunctions,
		modbusSlaveDefaultFunctionCount
	);
	assert(modbusIsOk(slave_error) && "slave init failed");

	master_error = modbusMasterInit(
		&master,
		dataCallback,
		masterExceptionCallback,
		modbusDefaultAllocator,
		modbusMasterDefaultFunctions,
		modbusMasterDefaultFunctionCount
	);
	assert(modbusIsOk(master_error) && "master init failed");

	reset();
	test_main();

	modbusMasterDestroy(&master);
	modbusSlaveDestroy(&slave);
	return 0;
}