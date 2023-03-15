#ifndef LIGHTMODBUS_SLAVE_FUNC_H
#define LIGHTMODBUS_SLAVE_FUNC_H

#include "base.h"
#include "slave.h"

/**
	\file slave_func.h
	\brief Slave's functions for parsing requests (header)
*/

LIGHTMODBUS_RET_ERROR modbusParseRequest01020304(
	ModbusSlave *status,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength);

LIGHTMODBUS_RET_ERROR modbusParseRequest0506(
	ModbusSlave *status,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength);

LIGHTMODBUS_RET_ERROR modbusParseRequest1516(
	ModbusSlave *status,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength);

LIGHTMODBUS_RET_ERROR modbusParseRequest22(
	ModbusSlave *status,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength);

#endif
