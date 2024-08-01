#ifndef LIGHTMODBUS_MASTER_IMPL_H
#define LIGHTMODBUS_MASTER_IMPL_H

#include "master.h"
#include "master_func.h"

/**
	\file master.impl.h
	\brief Master's types and basic functions (implementation)
*/

/**
	\brief Default array of supported functions. Length is stored in
	modbusMasterDefaultFunctionCount.

	Contents are controlled by defining `LIGHTMODBUS_FxxM` macros.
*/
ModbusMasterFunctionHandler modbusMasterDefaultFunctions[] =
{
#if defined(LIGHTMODBUS_F01M) || defined(LIGHTMODBUS_MASTER_FULL)
	{1, modbusParseResponse01020304},
#endif

#if defined(LIGHTMODBUS_F02M) || defined(LIGHTMODBUS_MASTER_FULL)
	{2, modbusParseResponse01020304},
#endif

#if defined(LIGHTMODBUS_F03M) || defined(LIGHTMODBUS_MASTER_FULL)
	{3, modbusParseResponse01020304},
#endif

#if defined(LIGHTMODBUS_F04M) || defined(LIGHTMODBUS_MASTER_FULL)
	{4, modbusParseResponse01020304},
#endif

#if defined(LIGHTMODBUS_F05M) || defined(LIGHTMODBUS_MASTER_FULL)
	{5, modbusParseResponse0506},
#endif

#if defined(LIGHTMODBUS_F06M) || defined(LIGHTMODBUS_MASTER_FULL)
	{6, modbusParseResponse0506},
#endif

#if defined(LIGHTMODBUS_F15M) || defined(LIGHTMODBUS_MASTER_FULL)
	{15, modbusParseResponse1516},
#endif

#if defined(LIGHTMODBUS_F16M) || defined(LIGHTMODBUS_MASTER_FULL)
	{16, modbusParseResponse1516},
#endif

#if defined(LIGHTMODBUS_F22M) || defined(LIGHTMODBUS_MASTER_FULL)
	{22, modbusParseResponse22},
#endif

#if defined(LIGHTMODBUS_F24M) || defined(LIGHTMODBUS_MASTER_FULL)
	{24, modbusParseResponse24},
#endif

	// Guard - prevents 0 size array
	{0, NULL}
};

/**
	\brief Stores length of modbusMasterDefaultFunctions array
*/
const uint8_t modbusMasterDefaultFunctionCount = sizeof(modbusMasterDefaultFunctions) / sizeof(modbusMasterDefaultFunctions[0]) - 1;

/**
	\brief Initializes a ModbusMaster struct
	\param status ModbusMaster struct to be initialized
	\param dataCallback Callback function for handling incoming data (may be required by used parsing functions)
	\param exceptionCallback Callback function for handling slave exceptions (optional)
	\param allocator Memory allocator to be used (see \ref modbusDefaultAllocator()) (required)
	\param functions Pointer to an array of supported function handlers (required).
		The lifetime of this array must not be shorter than the lifetime of the master.
	\param functionCount Number of elements in the `functions` array (required)
	\returns MODBUS_NO_ERROR() on success

	\see modbusDefaultAllocator()
	\see modbusMasterDefaultFunctions
*/
LIGHTMODBUS_RET_ERROR modbusMasterInit(
	ModbusMaster *status,
	ModbusDataCallback dataCallback,
	ModbusMasterExceptionCallback exceptionCallback,
	ModbusAllocator allocator,
	const ModbusMasterFunctionHandler *functions,
	uint8_t functionCount)
{
	status->dataCallback = dataCallback;
	status->exceptionCallback = exceptionCallback;
	status->functions = functions;
	status->functionCount = functionCount;
	status->context = NULL;

	return modbusBufferInit(&status->request, allocator);
}

/**
	\brief Deinitializes a ModbusMaster struct
	\param status ModbusMaster struct to be destroyed
	\note This does not free the memory pointed to by the `status` pointer and
	only cleans up the interals ofthe ModbusMaster struct.
*/
void modbusMasterDestroy(ModbusMaster *status)
{
	modbusBufferDestroy(&status->request, modbusMasterGetUserPointer(status));
}

/**
	\brief Begins a PDU-only request
	\returns MODBUS_NO_ERROR()
*/
LIGHTMODBUS_RET_ERROR modbusBeginRequestPDU(ModbusMaster *status)
{
	modbusBufferModePDU(&status->request);
	return MODBUS_NO_ERROR();
}

/**
	\brief Finalizes a PDU-only request
	\returns MODBUS_NO_ERROR()
*/
LIGHTMODBUS_RET_ERROR modbusEndRequestPDU(ModbusMaster *status)
{
	return MODBUS_NO_ERROR();
}

/**
	\brief Begins a RTU request
	\returns MODBUS_NO_ERROR()
*/
LIGHTMODBUS_RET_ERROR modbusBeginRequestRTU(ModbusMaster *status)
{
	modbusBufferModeRTU(&status->request);
	return MODBUS_NO_ERROR();
}

/**
	\brief Finalizes a Modbus RTU request
	\returns MODBUS_GENERAL_ERROR(LENGTH) if the allocated frame has invalid length
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusEndRequestRTU(ModbusMaster *status, uint8_t address)
{
	ModbusError err = modbusPackRTU(
		&status->request.data[0],
		status->request.length,
		address);

	if (err != MODBUS_OK)
		return MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_GENERAL, err);

	return MODBUS_NO_ERROR();
}

/**
	\brief Begins a TCP request
	\returns MODBUS_NO_ERROR()
*/
LIGHTMODBUS_RET_ERROR modbusBeginRequestTCP(ModbusMaster *status)
{
	modbusBufferModeTCP(&status->request);
	return MODBUS_NO_ERROR();
}

/**
	\brief Finalizes a Modbus TCP request
	\param transactionID Modbus TCP transaction identifier
	\param unitID Modbus TCP Unit ID
	\returns MODBUS_GENERAL_ERROR(LENGTH) if the allocated frame has invalid length
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusEndRequestTCP(ModbusMaster *status, uint16_t transactionID, uint8_t unitID)
{
	ModbusError err = modbusPackTCP(
		&status->request.data[0],
		status->request.length,
		transactionID,
		unitID);

	if (err != MODBUS_OK)
		return MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_GENERAL, err);

	return MODBUS_NO_ERROR();
}

/**
	\brief Parses a PDU section of a slave response
	\param address Value to be reported as slave address
	\param request Pointer to the PDU section of the request frame
	\param requestLength Length of the request PDU (valid range: 1 - 253)
	\param response Pointer to the PDU section of the response
	\param responseLength Length of the response PDU (valid range: 1 - 253)
	\returns MODBUS_REQUEST_ERROR(LENGTH) if the request has invalid length
	\returns MODBUS_RESPONSE_ERROR(LENGTH) if the response has invalid length
	\returns MODBUS_RESPONSE_ERROR(FUNCTION) if the function code in request doesn't match the one in response
	\returns MODBUS_GENERAL_ERROR(FUNCTION) if the function code is not supported
	\returns Result from the parsing function on success (modbusParseRequest*() functions)
*/
LIGHTMODBUS_RET_ERROR modbusParseResponsePDU(
	ModbusMaster *status,
	uint8_t address,
	const uint8_t *request,
	uint8_t requestLength,
	const uint8_t *response,
	uint8_t responseLength)
{
	// Check if lengths are ok
	if (!requestLength || requestLength > MODBUS_PDU_MAX)
		return MODBUS_REQUEST_ERROR(LENGTH);
	if (!responseLength || responseLength > MODBUS_PDU_MAX)
		return MODBUS_RESPONSE_ERROR(LENGTH);

	uint8_t function = response[0];

	// Handle exception frames
	if (function & 0x80 && responseLength == 2)
	{
		ModbusError error = MODBUS_ERROR_OTHER;
		if (status->exceptionCallback)
			error = status->exceptionCallback(
				status,
				address,
				function & 0x7f,
				(ModbusExceptionCode) response[1]);

		return MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_RESPONSE,  error);
	}

	// Check if function code matches the one in request frame
	if (function != request[0])
		return MODBUS_RESPONSE_ERROR(FUNCTION);

	// Find a parsing function
	for (uint16_t i = 0; i < status->functionCount; i++)
		if (function == status->functions[i].id)
			return status->functions[i].ptr(
				status,
				address,
				function,
				request,
				requestLength,
				response,
				responseLength);

	// No matching function handler
	return MODBUS_GENERAL_ERROR(FUNCTION);
}

/**
	\brief Parses a Modbus RTU slave response
	\param request Pointer to the request frame
	\param requestLength Length of the request (valid range: 4 - 256)
	\param response Pointer to the response frame
	\param responseLength Length of the response (valid range: 4 - 256)
	\returns MODBUS_REQUEST_ERROR(LENGTH) if the request has invalid length
	\returns MODBUS_RESPONSE_ERROR(LENGTH) if the response has invalid length
	\returns MODBUS_REQUEST_ERROR(CRC) if the request CRC is invalid
	\returns MODBUS_RESPONSE_ERROR(CRC) if the response CRC is invalid
	\returns MODBUS_RESPONSE_ERROR(ADDRESS) if the address is 0 or if request/response addressess don't match
	\returns Result of modbusParseResponsePDU() otherwise
*/
LIGHTMODBUS_RET_ERROR modbusParseResponseRTU(
	ModbusMaster *status,
	const uint8_t *request,
	uint16_t requestLength,
	const uint8_t *response,
	uint16_t responseLength)
{
	// Unpack request
	const uint8_t *requestPDU = nullptr;
	uint16_t requestPDULength = 0;
	uint8_t requestAddress = 0;
	ModbusError err = modbusUnpackRTU(
		request,
		requestLength,
#ifdef LIGHTMODBUS_MASTER_OMIT_REQUEST_CRC
		0,
#else
		1,
#endif
		&requestPDU,
		&requestPDULength,
		&requestAddress);

	if (err != MODBUS_OK)
		return MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_REQUEST, err);

	// Unpack response
	const uint8_t *responsePDU;
	uint16_t responsePDULength;
	uint8_t responseAddress;
	err = modbusUnpackRTU(
		response,
		responseLength,
		1,
		&responsePDU,
		&responsePDULength,
		&responseAddress);

	if (err != MODBUS_OK)
		return MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_RESPONSE, err);

	// Check addresses - response to a broadcast request or bad response address
	if (requestAddress == 0 || requestAddress != responseAddress)
		return MODBUS_RESPONSE_ERROR(ADDRESS);

	// Parse the PDU itself
	return modbusParseResponsePDU(
		status,
		requestAddress,
		requestPDU,
		requestPDULength,
		responsePDU,
		responsePDULength);
}

/**
	\brief Parses a Modbus TCP slave response
	\param request Pointer to the request frame
	\param requestLength Length of the request (valid range: 8 - 260)
	\param response Pointer to the response frame
	\param responseLength Length of the response (valid range: 8 - 260)
	\returns MODBUS_REQUEST_ERROR(LENGTH) if the request has invalid length or if the request frame has different from declared one
	\returns MODBUS_RESPONSE_ERROR(LENGTH) if the response has invalid length or if the response frame has different from declared one
	\returns MODBUS_REQUEST_ERROR(BAD_PROTOCOL) if the protocol ID in request is not 0
	\returns MODBUS_RESPONSE_ERROR(BAD_PROTOCOL) if the protocol ID in response is not 0
	\returns MODBUS_RESPONSE_ERROR(BAD_TRANSACTION) if the transaction ID in request is not the same as in response
	\returns MODBUS_RESPONSE_ERROR(ADDRESS) if the address in response is not the same as in request
	\returns Result of modbusParseResponsePDU() otherwise
*/
LIGHTMODBUS_RET_ERROR modbusParseResponseTCP(
	ModbusMaster *status,
	const uint8_t *request,
	uint16_t requestLength,
	const uint8_t *response,
	uint16_t responseLength)
{
	// Unpack request
	const uint8_t *requestPDU;
	uint16_t requestPDULength;
	uint16_t requestTransactionID;
	uint8_t requestUnitID;
	ModbusError err = modbusUnpackTCP(
		request,
		requestLength,
		&requestPDU,
		&requestPDULength,
		&requestTransactionID,
		&requestUnitID);

	if (err != MODBUS_OK)
		return MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_REQUEST, err);

	// Unpack response
	const uint8_t *responsePDU;
	uint16_t responsePDULength;
 	uint16_t responseTransactionID;
	uint8_t responseUnitID;
	err = modbusUnpackTCP(
		response,
		responseLength,
		&responsePDU,
		&responsePDULength,
		&responseTransactionID,
		&responseUnitID);

	if (err != MODBUS_OK)
		return MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_RESPONSE, err);

	// Check if transaction IDs match
	if (requestTransactionID != responseTransactionID)
		return MODBUS_RESPONSE_ERROR(BAD_TRANSACTION);

	// Check if unit IDs match
	if (requestUnitID != responseUnitID)
		return MODBUS_RESPONSE_ERROR(ADDRESS);

	// Parse the PDU itself
	return modbusParseResponsePDU(
		status,
		requestUnitID,
		requestPDU,
		requestPDULength,
		responsePDU,
		responsePDULength);
}

#endif
