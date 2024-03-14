#include <iostream>
#include <iomanip>
#define LIGHTMODBUS_FULL
#define LIGHTMODBUS_IMPL
#include <lightmodbus/lightmodbus.hpp>

ModbusError regCallback(
	const ModbusSlave *slave,
	const ModbusRegisterCallbackArgs *args,
	ModbusRegisterCallbackResult *result)
{
	switch (args->query)
	{
		// No write access
		case MODBUS_REGQ_W_CHECK:
			result->exceptionCode = MODBUS_EXCEP_SLAVE_FAILURE;
			break;

		// Read access to everything
		case MODBUS_REGQ_R_CHECK:
			result->exceptionCode = MODBUS_EXCEP_NONE;
			break;

		// Return 7 when reading
		case MODBUS_REGQ_R:
			result->value = 7;
			break;
		
		// Ignore write requests (should not happen)
		case MODBUS_REGQ_W:
			throw std::runtime_error{"this should never happen"};
			break;
	}

	// Always return MODBUS_OK
	return MODBUS_OK;
}

ModbusError dataCallback(const ModbusMaster *master, const ModbusDataCallbackArgs *args)
{
	// I'm sorry, C++ people... I'm too tired right now to use std::cout
	std::printf(
		"Received data:\n"
		"\t from: %d\n"
		"\t  fun: %d\n"
		"\t type: %s\n"
		"\t   id: %d\n"
		"\tvalue: %d\n",
		args->address,
		args->function,
		modbusDataTypeStr(args->type),
		args->index,
		args->value
	);

	// Always return MODBUS_OK
	return MODBUS_OK;
}

int main()
{
	llm::Slave slave(regCallback);
	llm::Master master(dataCallback);

	try
	{
		// Look how neat it looks
		master.buildRequest01RTU(4, 15, 3);
		slave.parseRequestRTU(4, master.getRequest(), master.getRequestLength());
		master.parseResponseRTU(
			master.getRequest(),
			master.getRequestLength(),
			slave.getResponse(),
			slave.getResponseLength());
	}
	catch (const llm::GeneralError &e)
	{
		std::cerr << "General error (serious): " << e.what() << std::endl;
	}
	catch (const llm::RequestError &e)
	{
		std::cerr << "Request error: " << e.what() << std::endl;
	}
	catch (const llm::ResponseError &e)
	{
		std::cerr << "Response error: " << e.what() << std::endl;
	}

	return 0;
}