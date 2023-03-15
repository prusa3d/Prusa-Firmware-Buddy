#define LIGHTMODBUS_FULL
#define LIGHTMODBUS_DEBUG
#define LIGHTMODBUS_IMPL
#include <lightmodbus/lightmodbus.h>
#include <stdio.h>
#include <assert.h>

/*
	A register callback that prints out everything what's happening
*/
ModbusError registerCallback(
	const ModbusSlave *slave,
	const ModbusRegisterCallbackArgs *args,
	ModbusRegisterCallbackResult *result)
{
	// Use functions from debug utilities to nicely
	// convert enum values to strings
	printf(
		"Register query:\n"
		"\tquery: %s\n"
		"\t type: %s\n"
		"\t   id: %d\n"
		"\tvalue: %d\n"
		"\t  fun: %d\n",
		modbusRegisterQueryStr(args->query),
		modbusDataTypeStr(args->type),
		args->index,
		args->value,
		args->function
	);

	switch (args->query)
	{
		// Pretend we allow all access
		// Tip: Change MODBUS_EXCEP_NONE to something else
		// 		and see what happens
		case MODBUS_REGQ_R_CHECK:
		case MODBUS_REGQ_W_CHECK:
			result->exceptionCode = MODBUS_EXCEP_NONE;
			break;

		// Return 7 when reading
		case MODBUS_REGQ_R:
			result->value = 7;
			break;
		
		// Ignore write requests
		case MODBUS_REGQ_W:
			break;
	}

	// Always return MODBUS_OK
	return MODBUS_OK;
}

/*
	Exception callback for printing out exceptions
*/
ModbusError slaveExceptionCallback(const ModbusSlave *slave, uint8_t function, ModbusExceptionCode code)
{
	printf("Slave exception %s (function %d)\n", modbusExceptionCodeStr(code), function);
	
	// Always return MODBUS_OK
	return MODBUS_OK;
}

/*
	Data callback for printing all incoming data
*/
ModbusError dataCallback(const ModbusMaster *master, const ModbusDataCallbackArgs *args)
{
	printf(
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

/*
	Exception callback for printing out exceptions on master side
*/
ModbusError masterExceptionCallback(const ModbusMaster *master, uint8_t address, uint8_t function, ModbusExceptionCode code)
{
	printf("Received slave %d exception %s (function %d)\n", address, modbusExceptionCodeStr(code), function);

	// Always return MODBUS_OK
	return MODBUS_OK;
}

/*
	Helper function for printing out frames
*/
void printBytes(const uint8_t *data, int length)
{
	for (int i = 0; i < length; i++)
		printf("0x%02x ", data[i]);
}

/*
	Helper function for printing out ModbusErrorInfo
*/
void printErrorInfo(ModbusErrorInfo err)
{
	if (modbusIsOk(err))
		printf("OK");
	else
		printf("%s: %s",
			modbusErrorSourceStr(modbusGetErrorSource(err)),
			modbusErrorStr(modbusGetErrorCode(err)));
}

int main(int argc, char *argv[])
{
	// Master and slave structs
	ModbusErrorInfo err;
	ModbusMaster master;
	ModbusSlave slave;

	// Init master
	err = modbusMasterInit(
		&master,
		dataCallback,
		masterExceptionCallback,
		modbusDefaultAllocator,
		modbusMasterDefaultFunctions,
		modbusMasterDefaultFunctionCount);
	printf("Slave init: "); printErrorInfo(err); printf("\n");
	assert(modbusIsOk(err) && "modbusMasterInit() failed!");

	// Init slave
	err = modbusSlaveInit(
		&slave,
		registerCallback,
		slaveExceptionCallback,
		modbusDefaultAllocator,
		modbusSlaveDefaultFunctions,
		modbusSlaveDefaultFunctionCount);
	printf("Master init: "); printErrorInfo(err); printf("\n");
	assert(modbusIsOk(err) && "modbusSlaveInit() failed!");
		
	// Read 2 registers starting at 78
	err = modbusBuildRequest03RTU(&master, 1, 78, 2);
	printf("Build request: "); printErrorInfo(err); printf("\n");
	assert(modbusIsOk(err) && "failed to build the request!");

	// Print out the request
	printf("Request: ");
	printBytes(modbusMasterGetRequest(&master), modbusMasterGetRequestLength(&master));
	printf("\n");

	// Let the slave parse this reqiest
	err = modbusParseRequestRTU(
		&slave,
		1,
		modbusMasterGetRequest(&master),
		modbusMasterGetRequestLength(&master));	
	printf("Parse request: "); printErrorInfo(err); printf("\n");
	
	// Check for any serious errors
	assert(modbusGetGeneralError(err) == MODBUS_OK && "slave error!");

	// Print out the response
	printf("Response: ");
	printBytes(modbusSlaveGetResponse(&slave), modbusSlaveGetResponseLength(&slave));
	printf("\n");

	// If the slave didn't respond, we're done here
	if (!modbusIsOk(err))
		goto cleanup;

	// Let the master parse the response
	err = modbusParseResponseRTU(
		&master,
		modbusMasterGetRequest(&master),
		modbusMasterGetRequestLength(&master),
		modbusSlaveGetResponse(&slave),
		modbusSlaveGetResponseLength(&slave));
	printf("Parse response: "); printErrorInfo(err); printf("\n");

	// Check for any serious errors
	assert(modbusGetGeneralError(err) == MODBUS_OK && "master error!");

	// Cleanup
	cleanup:
	modbusSlaveDestroy(&slave);
	modbusMasterDestroy(&master);
	return 0;
}
