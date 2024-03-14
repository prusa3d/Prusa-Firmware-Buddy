#include <stdio.h>
#include <assert.h>
#include "modbus.h"

ModbusError dataCallback(const ModbusMaster *master, const ModbusDataCallbackArgs *args)
{
	return MODBUS_OK;
}

int main(void)
{
	ModbusMaster master;
	ModbusErrorInfo err = modbusMasterInit(
		&master,
		dataCallback,
		NULL,
		modbusDefaultAllocator,
		modbusMasterDefaultFunctions,
		modbusMasterDefaultFunctionCount
	);
	assert(modbusIsOk(err));

	err = modbusBuildRequest01RTU(&master, 7, 16, 65);
	assert(modbusIsOk(err));

	for (int i = 0; i < modbusMasterGetRequestLength(&master); i++)
		printf("0x%02x ", modbusMasterGetRequest(&master)[i]);
	putchar('\n');

	modbusMasterDestroy(&master);
	return 0;
}