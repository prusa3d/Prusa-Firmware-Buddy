#include <stdio.h>
#include <assert.h>

#define LIGHTMODBUS_SLAVE
#define LIGHTMODBUS_IMPL
#include <lightmodbus/lightmodbus.h>

/*
	Responds with "ping\0" when called with even
	value and with "pong\0" if called with odd value.

	Reports an exception if the provided value is 0.
*/
LIGHTMODBUS_RET_ERROR parseMyRequest(
	ModbusSlave *status,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength)
{
	// Check request length
	if (requestLength < 2)
		return modbusBuildException(status, function, MODBUS_EXCEP_ILLEGAL_VALUE);

	// Check for the 'invalid' case
	if (requestPDU[1] == 0)
		return modbusBuildException(status, function, MODBUS_EXCEP_ILLEGAL_VALUE);
	
	// Allocate 6 bytes for the response. Handle memory allocation errors
	if (modbusSlaveAllocateResponse(status, 6) != MODBUS_OK)
		return MODBUS_GENERAL_ERROR(ALLOC);
	
	// Write data to the response frame
	// Be sure to write request.pdu and not request.data
	status->response.pdu[0] = function;
	status->response.pdu[1] = 'p';
	status->response.pdu[2] = requestPDU[1] % 2 ? 'o' : 'i';
	status->response.pdu[3] = 'n';
	status->response.pdu[4] = 'g';
	status->response.pdu[5] = 0;

	return MODBUS_NO_ERROR();
}

/*
	A dummy register callback
*/
ModbusError dummyRegCallback(
	const ModbusSlave *slave,
	const ModbusRegisterCallbackArgs *args,
	ModbusRegisterCallbackResult *result)
{
	return MODBUS_OK;
}

int main(void)
{
	// Handler array only containing our ping-pong function
	ModbusSlaveFunctionHandler functions[1] = 
	{
		{65, parseMyRequest}
	};

	// Init slave
	ModbusSlave slave;
	ModbusErrorInfo err = modbusSlaveInit(
		&slave,
		dummyRegCallback,
		NULL,
		modbusDefaultAllocator,
		functions,
		1);
	assert(modbusIsOk(err));

	// Request frame with odd number
	uint8_t request[] = {65, 33};

	// Parse the request
	err = modbusParseRequestPDU(&slave, request, sizeof request);
	assert(modbusIsOk(err));

	// Print the response
	for (int i = 0; i < modbusSlaveGetResponseLength(&slave); i++)
		printf("0x%02x ", modbusSlaveGetResponse(&slave)[i]);

	printf("\n");

	// Print the response as ASCII
	for (int i = 0; i < modbusSlaveGetResponseLength(&slave); i++)
		printf("%c ", modbusSlaveGetResponse(&slave)[i]);

	printf("\n");

	// Destroy slave
	modbusSlaveDestroy(&slave);
}