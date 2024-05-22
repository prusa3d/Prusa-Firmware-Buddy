#ifndef LIGHTMODBUS_MASTER_FUNC_IMPL_H
#define LIGHTMODBUS_MASTER_FUNC_IMPL_H

#include "master_func.h"
#include "master.h"

/**
	\file master_func.impl.h
	\brief Master's functions for building requests and parsing responses (implementation)
*/

/**
	\brief Parses response to requests 01, 02, 03 and 04
	\param address Address of the slave
	\param function Response function code
	\param requestPDU pointer to the PDU section of the request frame
	\param requestLength request PDU section length
	\param responsePDU pointer to the PDU section of the response frame
	\param responseLength response PDU section length
	\return MODBUS_REQUEST_ERROR(LENGTH) if request frame has invalid length
	\return MODBUS_RESPONSE_ERROR(LENGTH) if response frame has invalid length
	\return MODBUS_GENERAL_ERROR(FUNCTION) if `function` is not one of: 01, 02, 03, 04
	\return MODBUS_REQUEST_ERROR(COUNT) if the declared register count is invalid
	\return MODBUS_REQUEST_ERROR(RANGE) if the declared register range wraps around address space
	\return MODBUS_RESPONSE_ERROR(LENGTH) if the response length is not as expected
	\return MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusParseResponse01020304(
	ModbusMaster *status,
	uint8_t address,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength,
	const uint8_t *responsePDU,
	uint8_t responseLength)
{
	// Check if lengths are ok
	if (requestLength != 5) return MODBUS_REQUEST_ERROR(LENGTH);
	if (responseLength < 3) return MODBUS_RESPONSE_ERROR(LENGTH);

	// Determine data type
	uint8_t bits;
	uint16_t maxCount;
	ModbusDataType datatype;
	switch (function)
	{
		case 1:
			datatype = MODBUS_COIL;
			maxCount = 2000;
			bits = 1;
			break;

		case 2:
			datatype = MODBUS_DISCRETE_INPUT;
			maxCount = 2000;
			bits = 1;
			break;

		case 3:
			datatype = MODBUS_HOLDING_REGISTER;
			maxCount = 125;
			bits = 16;
			break;

		case 4:
			datatype = MODBUS_INPUT_REGISTER;
			maxCount = 125;
			bits = 16;
			break;
		
		default:
			return MODBUS_GENERAL_ERROR(FUNCTION);
	}

	uint16_t index = modbusRBE(&requestPDU[1]);
	uint16_t count = modbusRBE(&requestPDU[3]);

	// Check count
	if (count == 0 || count > maxCount)
		return MODBUS_REQUEST_ERROR(COUNT);

	// Address range check
	if (modbusCheckRangeU16(index, count))
		return MODBUS_REQUEST_ERROR(RANGE);

	// Based on the request, calculate expected data size
	uint8_t expected = (bits == 16) ? (count << 1) : modbusBitsToBytes(count);

	// Check if declared data size matches
	// and if response length is valid
	if (responsePDU[1] != expected || responseLength != expected + 2)
		return MODBUS_RESPONSE_ERROR(LENGTH);

	// Prepare callback args
	ModbusDataCallbackArgs cargs = {
		.type = datatype,
		.index = 0,
		.value = 0,
		.function = function,
		.address = address,
	};

	// And finally read the data from the response
	for (uint16_t i = 0; i < count; i++)
	{
		cargs.index = index + i;
		if (bits == 1)
			cargs.value = modbusMaskRead(&responsePDU[2], i);
		else
			cargs.value = modbusRBE(&responsePDU[2 + (i << 1)]);

		status->dataCallback(status, &cargs);
	}

	return MODBUS_NO_ERROR();
}

/**
	\brief Parses response to requests 05 and 06
	\param address Address of the slave
	\param function Response function code
	\param requestPDU pointer to the PDU section of the request frame
	\param requestLength request PDU section length
	\param responsePDU pointer to the PDU section of the response frame
	\param responseLength response PDU section length
	\return MODBUS_REQUEST_ERROR(LENGTH) if request frame has invalid length
	\return MODBUS_RESPONSE_ERROR(LENGTH) if response frame has invalid length
	\return MODBUS_RESPONSE_ERROR(INDEX) if the register index is different in request and response
	\return MODBUS_RESPONSE_ERROR(VALUE) if the register value is different in request and response
	\return MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusParseResponse0506(
	ModbusMaster *status,
	uint8_t address,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength,
	const uint8_t *responsePDU,
	uint8_t responseLength)
{
	// Check if lengths are ok
	if (requestLength != 5)	return MODBUS_REQUEST_ERROR(LENGTH);
	if (responseLength != 5) return MODBUS_RESPONSE_ERROR(LENGTH);

	// Verify index
	if (modbusRBE(&requestPDU[1]) != modbusRBE(&responsePDU[1]))
		return MODBUS_RESPONSE_ERROR(INDEX);

	// Verify value
	if (modbusRBE(&requestPDU[3]) != modbusRBE(&responsePDU[3]))
		return MODBUS_RESPONSE_ERROR(VALUE);

	return MODBUS_NO_ERROR();
}

/**
	\brief Parses response to requests 15 and 16 (write mutliple regsiters/coils)
	\param address Address of the slave
	\param function Response function code
	\param requestPDU pointer to the PDU section of the request frame
	\param requestLength request PDU section length
	\param responsePDU pointer to the PDU section of the response frame
	\param responseLength response PDU section length
	\return MODBUS_REQUEST_ERROR(LENGTH) if request frame has invalid length
	\return MODBUS_RESPONSE_ERROR(LENGTH) if response frame has invalid length
	\return MODBUS_REQUEST_ERROR(COUNT) if the declared register count is invalid
	\return MODBUS_REQUEST_ERROR(RANGE) if the declared register range wraps around address space
	\return MODBUS_RESPONSE_ERROR(INDEX) if the index differs between the request and response
	\return MODBUS_RESPONSE_ERROR(COUNT) if the count differs between the request and response
	\return MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusParseResponse1516(
	ModbusMaster *status,
	uint8_t address,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength,
	const uint8_t *responsePDU,
	uint8_t responseLength)
{
	// Check if lengths are ok
	if (requestLength < 7) return MODBUS_REQUEST_ERROR(LENGTH);
	if (responseLength != 5) return MODBUS_RESPONSE_ERROR(LENGTH);

	uint16_t index = modbusRBE(&requestPDU[1]);
	uint16_t count = modbusRBE(&requestPDU[3]);

	// Check if index is the same in the response
	if (index != modbusRBE(&responsePDU[1]))
		return MODBUS_RESPONSE_ERROR(INDEX);
		
	// Check if count is the same
	if (count != modbusRBE(&responsePDU[3]))
		return MODBUS_RESPONSE_ERROR(COUNT);

	// Verify count in the request
	uint16_t maxCount = (function == 15) ? 1968 : 123;
	if (count == 0 || count > maxCount)
		return MODBUS_REQUEST_ERROR(COUNT);
	
	// Verify if the request length is correct
	uint16_t expected = function == 15 ? modbusBitsToBytes(count) : (count << 1);
	if (requestLength != expected + 6)
		return MODBUS_REQUEST_ERROR(LENGTH);

	// Verify register range
	if (modbusCheckRangeU16(index, count))
		return MODBUS_REQUEST_ERROR(RANGE);
	
	return MODBUS_NO_ERROR();
}

/**
	\brief Parses response to request 22
	\param address Address of the slave
	\param function Response function code
	\param requestPDU pointer to the PDU section of the request frame
	\param requestLength request PDU section length
	\param responsePDU pointer to the PDU section of the response frame
	\param responseLength response PDU section length
	\return MODBUS_REQUEST_ERROR(LENGTH) if request frame has invalid length
	\return MODBUS_RESPONSE_ERROR(LENGTH) if response frame has invalid length
	\return MODBUS_RESPONSE_ERROR(OTHER) if the response is different from the request
	\return MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusParseResponse22(
	ModbusMaster *status,
	uint8_t address,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength,
	const uint8_t *responsePDU,
	uint8_t responseLength)
{
	// Check lengths
	if (requestLength != 7) return MODBUS_REQUEST_ERROR(LENGTH);
	if (responseLength != 7) return MODBUS_RESPONSE_ERROR(LENGTH);;
	
	// The response should be identical to the request
	uint8_t ok = 1;
	for (uint8_t i = 0; ok && i < 7; i++)
		ok = ok && (responsePDU[i] == requestPDU[i]);
	
	if (!ok) return MODBUS_RESPONSE_ERROR(OTHER);

	return MODBUS_NO_ERROR();
}

/**
	\brief Parses response to request 22
	\param address Address of the slave
	\param function Response function code
	\param requestPDU pointer to the PDU section of the request frame
	\param requestLength request PDU section length
	\param responsePDU pointer to the PDU section of the response frame
	\param responseLength response PDU section length
	\return MODBUS_REQUEST_ERROR(LENGTH) if request frame has invalid length
	\return MODBUS_RESPONSE_ERROR(LENGTH) if response frame has invalid length
	\return MODBUS_RESPONSE_ERROR(OTHER) if the response is different from the request
	\return MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusParseResponse24(
	ModbusMaster *status,
	uint8_t address,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength,
	const uint8_t *responsePDU,
	uint8_t responseLength)
{
	// Check if lengths are ok
	if (requestLength != 5) return MODBUS_REQUEST_ERROR(LENGTH);
	if (responseLength < 3) return MODBUS_RESPONSE_ERROR(LENGTH);

	// Check function
	if (function != 24) return MODBUS_RESPONSE_ERROR(FUNCTION);

	// Get FIFO pointer address
	uint16_t pointer_address = modbusRBE(&requestPDU[1]);

	// Get number of registers in response
	uint16_t count = modbusRBE(&responsePDU[3]);

	// Check count
	if (count > 31) // 31 register according to Modbus specification
		return MODBUS_REQUEST_ERROR(COUNT);

	// Prepare callback args
	ModbusDataCallbackArgs cargs = {
		.type = MODBUS_FIFO_INPUT,
		.index = pointer_address,
		.value = 0,
		.function = function,
		.address = address,
		.offset = 0,
	};

	// And finally read the data from the response
	for (uint16_t i = 0; i < count; i++) {
		cargs.offset = i;
		cargs.value = modbusRBE(&responsePDU[2 * i + 5]);
		status->dataCallback(status, &cargs);
	}

	return MODBUS_NO_ERROR();
}

/**
	\brief Read mutiple coils/discrete inputs/holding registers/input registers
	\param function 1 to read coils, 2 to read discrete inputs, 3 to read holding registers, 4 to read input registers
	\param index Index of the register to be read
	\param count Number of registers to be read
	\param value New value for the register/coil
	\returns MODBUS_GENERAL_ERROR(FUNCTION) if function is not 1, 2, 3 or 4
	\returns MODBUS_GENERAL_ERROR(COUNT) if count is zero or too large
	\returns MODBUS_GENERAL_ERROR(RANGE) if the register range wraps aroudthe register space
	\returns MODBUS_GENERAL_ERROR(ALLOC) on memory allocation error
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusBuildRequest01020304(
	ModbusMaster *status,
	uint8_t function,
	uint16_t index,
	uint16_t count)
{
	uint16_t maxCount;
	switch (function)
	{
		case 1:
		case 2:
			maxCount = 2000;
			break;

		case 3:
		case 4:
			maxCount = 125;
			break;
		
		default:
			return MODBUS_GENERAL_ERROR(FUNCTION);
	}

	// Check count
	if (count == 0 || count > maxCount)
		return MODBUS_GENERAL_ERROR(COUNT);
	
	// Address range check
	if (modbusCheckRangeU16(index, count))
		return MODBUS_GENERAL_ERROR(RANGE);

	if (modbusMasterAllocateRequest(status, 5))
		return MODBUS_GENERAL_ERROR(ALLOC);
	
	status->request.pdu[0] = function;
	modbusWBE(&status->request.pdu[1], index);
	modbusWBE(&status->request.pdu[3], count);

	return MODBUS_NO_ERROR();
}

/**
	\brief Write single coil/holding register
	\param function 5 to write a coil, 6 to write a holding register
	\param index Index of the register/coil to be written
	\param value New value for the register/coil
	\returns MODBUS_GENERAL_ERROR(FUNCTION) if `function` is not 5 or 6
	\returns MODBUS_GENERAL_ERROR(ALLOC) on memory allocation error
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusBuildRequest0506(
	ModbusMaster *status,
	uint8_t function,
	uint16_t index,
	uint16_t value)
{
	if (function != 5 && function != 6)
		return MODBUS_GENERAL_ERROR(FUNCTION);

	// Write coils using proper value
	if (function == 5)
		value = value ? 0xff00 : 0;

	if (modbusMasterAllocateRequest(status, 5))
		return MODBUS_GENERAL_ERROR(ALLOC);
	
	status->request.pdu[0] = function;
	modbusWBE(&status->request.pdu[1], index);
	modbusWBE(&status->request.pdu[3], value);

	return MODBUS_NO_ERROR();
}

/**
	\brief Write multiple coils
	\param index Index of the first coil to be written
	\param count Number of coils to be written
	\param values Pointer to array containing `count` coil values (each bit corresponds to one coil value)
	\returns MODBUS_GENERAL_ERROR(COUNT) if `count` is zero or too large
	\returns MODBUS_GENERAL_ERROR(RANGE) if the register range wraps around the register space
	\returns MODBUS_GENERAL_ERROR(ALLOC) on memory allocation error
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusBuildRequest15(
	ModbusMaster *status,
	uint16_t index,
	uint16_t count,
	const uint8_t *values)
{
	// Check count
	if (count == 0 || count > 1968)
		return MODBUS_GENERAL_ERROR(COUNT);

	// Address range check
	if (modbusCheckRangeU16(index, count))
		return MODBUS_GENERAL_ERROR(RANGE);

	uint8_t dataLength = modbusBitsToBytes(count);

	if (modbusMasterAllocateRequest(status, 6 + dataLength))
		return MODBUS_GENERAL_ERROR(ALLOC);

	// Number of full bytes and remaining bits
	uint8_t n = count >> 3;
	uint8_t r = count & 7;

	// Copy n full bytes
	for (uint8_t i = 0; i < n; i++)
		status->request.pdu[6 + i] = values[i];

	// Copy remaining bits
	if (r)
	{
		status->request.pdu[6 + n] = 0;
		for (uint8_t i = 0; i < r; i++)
			modbusMaskWrite(
				&status->request.pdu[6 + n], 
				i,
				modbusMaskRead(values + n, i));
	}

	status->request.pdu[0] = 15;
	modbusWBE(&status->request.pdu[1], index);
	modbusWBE(&status->request.pdu[3], count);
	status->request.pdu[5] = dataLength;
	
	return MODBUS_NO_ERROR();
}

/**
	\brief Write multiple holding registers
	\param index Index of the first register to be written
	\param count Number of registers to be written
	\param values Pointer to array containing `count` register values. Each 16-bit word corresponds to one register value
	\returns MODBUS_GENERAL_ERROR(COUNT) if `count` is zero or too large
	\returns MODBUS_GENERAL_ERROR(RANGE) if the register range wraps around the register space
	\returns MODBUS_GENERAL_ERROR(ALLOC) on memory allocation error
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusBuildRequest16(
	ModbusMaster *status,
	uint16_t index,
	uint16_t count,
	const uint16_t *values)
{
	// Check count
	if (count == 0 || count > 123)
		return MODBUS_GENERAL_ERROR(COUNT);

	// Addresss range check
	if (modbusCheckRangeU16(index, count))
		return MODBUS_GENERAL_ERROR(RANGE);

	uint8_t dataLength = count << 1;

	if (modbusMasterAllocateRequest(status, 6 + dataLength))
		return MODBUS_GENERAL_ERROR(ALLOC);

	// Copy register values
	for (uint8_t i = 0; i < (uint8_t)count; i++)
		modbusWBE(&status->request.pdu[6 + (i << 1)], values[i]);

	status->request.pdu[0] = 16;
	modbusWBE(&status->request.pdu[1], index);
	modbusWBE(&status->request.pdu[3], count);
	status->request.pdu[5] = dataLength;
	return MODBUS_NO_ERROR();
}

/**
	\brief Mask write register request
	\param index Register ID
	\param andmax AND mask
	\param ormask OR mask
	\returns MODBUS_GENERAL_ERROR(ALLOC) on memory allocation error
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusBuildRequest22(
	ModbusMaster *status,
	uint16_t index,
	uint16_t andmask,
	uint16_t ormask)
{
	if (modbusMasterAllocateRequest(status, 7))
		return MODBUS_GENERAL_ERROR(ALLOC);
	
	status->request.pdu[0] = 22;
	modbusWBE(&status->request.pdu[1], index);
	modbusWBE(&status->request.pdu[3], andmask);
	modbusWBE(&status->request.pdu[5], ormask);

	return MODBUS_NO_ERROR();
}

/**
	\brief Read FIFO request
	\param index Register ID
	\returns MODBUS_GENERAL_ERROR(ALLOC) on memory allocation error
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusBuildRequest24(
	ModbusMaster *status,
	uint16_t index)
{
	if (modbusMasterAllocateRequest(status, 5)) {
		return MODBUS_GENERAL_ERROR(ALLOC);
	}

	status->request.pdu[0] = 24;
	modbusWBE(&status->request.pdu[1], index);

	return MODBUS_NO_ERROR();
}

#endif
