#define LIGHTMODBUS_SLAVE_FULL
#define LIGHTMODBUS_DEBUG
#define LIGHTMODBUS_IMPL
#include <lightmodbus/lightmodbus.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

ModbusError registerCallback(
	const ModbusSlave *slave,
	const ModbusRegisterCallbackArgs *args,
	ModbusRegisterCallbackResult *result)
{
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
		// Pretend to allow all access
		case MODBUS_REGQ_R_CHECK:
		case MODBUS_REGQ_W_CHECK:
			result->exceptionCode = MODBUS_EXCEP_NONE;
			break;

		// Return 7 when reading
		case MODBUS_REGQ_R:
			result->value = 7;
			break;
		
		default: break;
	}

	return MODBUS_OK;
}

ModbusError exceptionCallback(const ModbusSlave *slave,  uint8_t function, ModbusExceptionCode code)
{
	printf("Slave exception %s (function %d)\n", modbusExceptionCodeStr(code), function);
	return MODBUS_OK;
}

void printErrorInfo(ModbusErrorInfo err)
{
	if (modbusIsOk(err))
		printf("OK");
	else
		printf("%s: %s",
			modbusErrorSourceStr(modbusGetErrorSource(err)),
			modbusErrorStr(modbusGetErrorCode(err)));
}

void printResponse(const ModbusSlave *slave)
{
	for (int i = 0; i < modbusSlaveGetResponseLength(slave); i++)
		printf("0x%02x ", modbusSlaveGetResponse(slave)[i]);
}

int main(int argc, char *argv[])
{
	uint8_t data[1024];
	int length = 0;

	// Read the data from stdin
	printf("Reading hex data from stdin...\n");
	for (int c; length < sizeof(data) && scanf("%x", &c) > 0; length++)
		data[length] = c;

	// Create a slave
	ModbusErrorInfo err;
	ModbusSlave slave;
	err = modbusSlaveInit(
		&slave,
		registerCallback,
		exceptionCallback,
		modbusDefaultAllocator,
		modbusSlaveDefaultFunctions,
		modbusSlaveDefaultFunctionCount);
	
	// Check for errors
	assert(modbusIsOk(err) && "modbusSlaveInit() failed!");

	printf("Parsing %d bytes\n", length);

	// Try to parse as PDU
	printf("\n\n------------------ PDU -----------------------\n");
	err = modbusParseRequestPDU(&slave, data, length);
	printErrorInfo(err);
	printf("\nPDU response: ");
	if (modbusIsOk(err)) printResponse(&slave);

	// Try to parse as RTU
	printf("\n\n------------------ RTU -----------------------\n");
	err = modbusParseRequestRTU(&slave, 1, data, length);
	printErrorInfo(err);
	printf("\nRTU response: ");
	if (modbusIsOk(err)) printResponse(&slave);

	// Try to parse as TCP
	printf("\n\n------------------ TCP -----------------------\n");
	err = modbusParseRequestTCP(&slave, data, length);
	printErrorInfo(err);
	printf("\nTCP response: ");
	if (modbusIsOk(err)) printResponse(&slave);

	// Cleanup
	modbusSlaveDestroy(&slave);

	printf("\n");
	return 0;
}
