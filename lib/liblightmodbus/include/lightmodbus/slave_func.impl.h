#ifndef LIGHTMODBUS_SLAVE_FUNC_IMPL_H
#define LIGHTMODBUS_SLAVE_FUNC_IMPL_H

#include "slave_func.h"
#include "slave.h"

/**
	\file slave_func.impl.h
	\brief Slave's functions for parsing requests (implementation)
*/

/**
	\brief Handles requests 01, 02, 03 and 04 (Read Multiple XX) and generates response.
	\param function function code
	\param requestPDU pointer to the PDU section of the request
	\param requestLength length of the PDU section in bytes
	\returns MODBUS_GENERAL_ERROR(ALLOC) on memory allocation error
	\returns MODBUS_GENERAL_ERROR(FUNCTION) if function is not 1, 2, 3 or 4
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusParseRequest01020304(
	ModbusSlave *status,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength)
{
	// Check frame length
	if (requestLength != 5)
		return modbusBuildException(status, function, MODBUS_EXCEP_ILLEGAL_VALUE);

	ModbusDataType datatype;
	uint16_t maxCount;
	uint8_t isCoilType;
	switch (function)
	{
		case 1:
			datatype = MODBUS_COIL;
			maxCount = 2000;
			isCoilType = 1;
			break;

		case 2:
			datatype = MODBUS_DISCRETE_INPUT;
			maxCount = 2000;
			isCoilType = 1;
			break;

		case 3:
			datatype = MODBUS_HOLDING_REGISTER;
			maxCount = 125;
			isCoilType = 0;
			break;

		case 4:
			datatype = MODBUS_INPUT_REGISTER;
			maxCount = 125;
			isCoilType = 0;
			break;
		
		default:
			return MODBUS_GENERAL_ERROR(FUNCTION);
			break;
	}

	uint16_t index = modbusRBE(&requestPDU[1]);
	uint16_t count = modbusRBE(&requestPDU[3]);

	// Check count
	if (count == 0 || count > maxCount)
		return modbusBuildException(status, function, MODBUS_EXCEP_ILLEGAL_VALUE);

	// Addresss range check
	if (modbusCheckRangeU16(index, count))
		return modbusBuildException(status, function, MODBUS_EXCEP_ILLEGAL_ADDRESS);

	// Prepare callback args
	ModbusRegisterCallbackResult cres;
	ModbusRegisterCallbackArgs cargs = {
		.type = datatype,
		.query = MODBUS_REGQ_R_CHECK,
		.index = 0,
		.value = 0,
		.function = function,
	};

	// Check if all registers can be read
	for (uint16_t i = 0; i < count; i++)
	{
		cargs.index = index + i;
		ModbusError fail = status->registerCallback(status, &cargs, &cres);
		if (fail) return modbusBuildException(status, function, MODBUS_EXCEP_SLAVE_FAILURE);
		if (cres.exceptionCode) return modbusBuildException(status, function, cres.exceptionCode);
	}

	// ---- RESPONSE ----

	uint8_t dataLength = (isCoilType ? modbusBitsToBytes(count) : (count << 1));
	if (modbusSlaveAllocateResponse(status, 2 + dataLength))
		return MODBUS_GENERAL_ERROR(ALLOC);

	status->response.pdu[0] = function;
	status->response.pdu[1] = dataLength;
	
	// Clear with zeros, if we're writing bits
	for (uint8_t i = 0; i < dataLength; i++)
		status->response.pdu[2 + i] = 0;

	cargs.query = MODBUS_REGQ_R;
	for (uint16_t i = 0; i < count; i++)
	{
		cargs.index = index + i;
		(void) status->registerCallback(status, &cargs, &cres);
		
		if (isCoilType)
			modbusMaskWrite(&status->response.pdu[2], i, cres.value != 0);
		else
			modbusWBE(&status->response.pdu[2 + (i << 1)], cres.value);
	}

	return MODBUS_NO_ERROR();
}

/**
	\brief Handles requests 05 and 06 (Write Single XX) and generates response.
	\param function function code
	\param requestPDU pointer to the PDU section of the request
	\param requestLength length of the PDU section in bytes
	\returns MODBUS_GENERAL_ERROR(ALLOC) on memory allocation error
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusParseRequest0506(
	ModbusSlave *status,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength)
{
	// Check frame length
	if (requestLength != 5)
		return modbusBuildException(status, function, MODBUS_EXCEP_ILLEGAL_VALUE);

	// Get register index and value
	ModbusDataType datatype = function == 5 ? MODBUS_COIL : MODBUS_HOLDING_REGISTER;
	uint16_t index = modbusRBE(&requestPDU[1]);
	uint16_t value = modbusRBE(&requestPDU[3]);

	// For coils - check if coil value is valid
	if (datatype == MODBUS_COIL && value != 0x0000 && value != 0xFF00)
		return modbusBuildException(status, function, MODBUS_EXCEP_ILLEGAL_VALUE);

	// Prepare callback args
	ModbusRegisterCallbackResult cres;
	ModbusRegisterCallbackArgs cargs = {
		.type = datatype,
		.query = MODBUS_REGQ_W_CHECK,
		.index = index,
		.value = (uint16_t)((datatype == MODBUS_COIL) ? (value != 0) : value),
		.function = function,
	};

	// Check if the register/coil can be written
	ModbusError fail = status->registerCallback(status, &cargs, &cres);
	if (fail) return modbusBuildException(status, function, MODBUS_EXCEP_SLAVE_FAILURE);
	if (cres.exceptionCode) return modbusBuildException(status, function, cres.exceptionCode);

	// Write coil/register
	// Keep in mind that 0xff00 is 0 when cast to uint8_t
	cargs.query = MODBUS_REGQ_W;
	(void) status->registerCallback(status, &cargs, &cres);

	// ---- RESPONSE ----

	if (modbusSlaveAllocateResponse(status, 5))
		return MODBUS_GENERAL_ERROR(ALLOC);

	status->response.pdu[0] = function;
	modbusWBE(&status->response.pdu[1], index);
	modbusWBE(&status->response.pdu[3], value);

	return MODBUS_NO_ERROR();
}

/**
	\brief Handles requests 15 and 16 (Write Multiple XX) and generates response.
	\param function function code
	\param requestPDU pointer to the PDU section of the request
	\param requestLength length of the PDU section in bytes
	\returns MODBUS_GENERAL_ERROR(ALLOC) on memory allocation error
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusParseRequest1516(
	ModbusSlave *status,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength)
{
	// Check length
	if (requestLength < 6)
		return modbusBuildException(status, function, MODBUS_EXCEP_ILLEGAL_VALUE);

	// Get first index and register count
	ModbusDataType datatype = function == 15 ? MODBUS_COIL : MODBUS_HOLDING_REGISTER;
	uint16_t maxCount = datatype == MODBUS_COIL ? 1968 : 123;
	uint16_t index = modbusRBE(&requestPDU[1]);
	uint16_t count = modbusRBE(&requestPDU[3]);
	uint8_t declaredLength = requestPDU[5];

	// Check if the declared length is correct
	if (declaredLength == 0 || declaredLength != requestLength - 6)
		return modbusBuildException(status, function, MODBUS_EXCEP_ILLEGAL_VALUE);

	// Check count
	if (count == 0
		|| count > maxCount
		|| declaredLength != (datatype == MODBUS_COIL ? modbusBitsToBytes(count) : (count << 1)))
		return modbusBuildException(status, function, MODBUS_EXCEP_ILLEGAL_VALUE);

	// Addresss range check
	if (modbusCheckRangeU16(index, count))
		return modbusBuildException(status, function, MODBUS_EXCEP_ILLEGAL_ADDRESS);

	// Prepare callback args
	ModbusRegisterCallbackResult cres;
	ModbusRegisterCallbackArgs cargs = {
		.type = datatype,
		.query = MODBUS_REGQ_W_CHECK,
		.index = 0,
		.value = 0,
		.function = function,
	};

	// Check write access
	for (uint16_t i = 0; i < count; i++)
	{
		cargs.index = index + i;
		cargs.value = datatype == MODBUS_COIL ? modbusMaskRead(&requestPDU[6], i) : modbusRBE(&requestPDU[6 + (i << 1)]);
		ModbusError fail = status->registerCallback(status, &cargs, &cres);
		if (fail) return modbusBuildException(status, function, MODBUS_EXCEP_SLAVE_FAILURE);
		if (cres.exceptionCode) return modbusBuildException(status, function, cres.exceptionCode);
	}

	// Write coils
	cargs.query = MODBUS_REGQ_W;
	for (uint16_t i = 0; i < count; i++)
	{
		cargs.index = index + i;
		cargs.value = datatype == MODBUS_COIL ? modbusMaskRead(&requestPDU[6], i) : modbusRBE(&requestPDU[6 + (i << 1)]);
		(void) status->registerCallback(status, &cargs, &cres);
	}

	// ---- RESPONSE ----

	if (modbusSlaveAllocateResponse(status, 5))
		return MODBUS_GENERAL_ERROR(ALLOC);

	status->response.pdu[0] = function;
	modbusWBE(&status->response.pdu[1], index);
	modbusWBE(&status->response.pdu[3], count);	

	return MODBUS_NO_ERROR();
}

/**
	\brief Handles request 22 (Mask Write Register) and generates response.
	\param function function code
	\param requestPDU pointer to the PDU section of the request
	\param requestLength length of the PDU section in bytes
	\returns MODBUS_GENERAL_ERROR(ALLOC) on memory allocation error
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusParseRequest22(
	ModbusSlave *status,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength)
{
	// Check length	
	if (requestLength != 7)
		return modbusBuildException(status, function, MODBUS_EXCEP_ILLEGAL_VALUE);

	// Get index and masks
	uint16_t index   = modbusRBE(&requestPDU[1]);
	uint16_t andmask = modbusRBE(&requestPDU[3]);
	uint16_t ormask  = modbusRBE(&requestPDU[5]);

	// Prepare callback args
	ModbusRegisterCallbackResult cres;
	ModbusRegisterCallbackArgs cargs = {
		.type = MODBUS_HOLDING_REGISTER,
		.query = MODBUS_REGQ_R_CHECK,
		.index = index,
		.value = 0,
		.function = function,
	};

	// Check read access
	cargs.query = MODBUS_REGQ_R_CHECK;
	ModbusError fail = status->registerCallback(status, &cargs, &cres);
	if (fail) return modbusBuildException(status, function, MODBUS_EXCEP_SLAVE_FAILURE);
	if (cres.exceptionCode) return modbusBuildException(status, function, cres.exceptionCode);

	// Read the register
	cargs.query = MODBUS_REGQ_R;
	(void) status->registerCallback(status, &cargs, &cres);
	uint16_t value = cres.value;

	// Compute new value for the register
	value = (value & andmask) | (ormask & ~andmask);

	// Check write access
	cargs.query = MODBUS_REGQ_W_CHECK;
	cargs.value = value;
	fail = status->registerCallback(status, &cargs, &cres);
	if (fail) return modbusBuildException(status, function, MODBUS_EXCEP_SLAVE_FAILURE);
	if (cres.exceptionCode) return modbusBuildException(status, function, cres.exceptionCode);

	// Write the register
	cargs.query = MODBUS_REGQ_W;
	(void) status->registerCallback(status, &cargs, &cres);
	
	// ---- RESPONSE ----

	if (modbusSlaveAllocateResponse(status, 7))
		return MODBUS_GENERAL_ERROR(ALLOC);

	status->response.pdu[0] = function;
	modbusWBE(&status->response.pdu[1], index);
	modbusWBE(&status->response.pdu[3], andmask);
	modbusWBE(&status->response.pdu[5], ormask);
	
	return MODBUS_NO_ERROR();
}

#endif
