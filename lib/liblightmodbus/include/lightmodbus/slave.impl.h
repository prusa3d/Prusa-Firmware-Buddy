#ifndef LIGHTMODBUS_SLAVE_IMPL_H
#define LIGHTMODBUS_SLAVE_IMPL_H

#include "slave.h"
#include "slave_func.h"

/**
	\file slave.impl.h
	\brief Slave's types and basic functions (implementation)
*/

/**
	\brief Associates function IDs with pointers to functions responsible
	for parsing. Length of this array is stored in modbusSlaveDefaultFunctionCount

	\note Contents depend on defined `LIGHTMODBUS_FxxS` macros!
*/
ModbusSlaveFunctionHandler modbusSlaveDefaultFunctions[] =
{
#if defined(LIGHTMODBUS_F01S) || defined(LIGHTMODBUS_SLAVE_FULL)
	{1, modbusParseRequest01020304},
#endif

#if defined(LIGHTMODBUS_F02S) || defined(LIGHTMODBUS_SLAVE_FULL)
	{2, modbusParseRequest01020304},
#endif

#if defined(LIGHTMODBUS_F03S) || defined(LIGHTMODBUS_SLAVE_FULL)
	{3, modbusParseRequest01020304},
#endif

#if defined(LIGHTMODBUS_F04S) || defined(LIGHTMODBUS_SLAVE_FULL)
	{4, modbusParseRequest01020304},
#endif

#if defined(LIGHTMODBUS_F05S) || defined(LIGHTMODBUS_SLAVE_FULL)
	{5, modbusParseRequest0506},
#endif

#if defined(LIGHTMODBUS_F06S) || defined(LIGHTMODBUS_SLAVE_FULL)
	{6, modbusParseRequest0506},
#endif

#if defined(LIGHTMODBUS_F15S) || defined(LIGHTMODBUS_SLAVE_FULL)
	{15, modbusParseRequest1516},
#endif

#if defined(LIGHTMODBUS_F16S) || defined(LIGHTMODBUS_SLAVE_FULL)
	{16, modbusParseRequest1516},
#endif

#if defined(LIGHTMODBUS_F22S) || defined(LIGHTMODBUS_SLAVE_FULL)
	{22, modbusParseRequest22},
#endif

	// Guard - prevents 0 array size
	{0, NULL}
};

/**
	\brief Stores length of modbusSlaveDefaultFunctions
*/
const uint8_t modbusSlaveDefaultFunctionCount = sizeof(modbusSlaveDefaultFunctions) / sizeof(modbusSlaveDefaultFunctions[0]) - 1;

/**
	\brief Initializes slave device
	\param registerCallback Callback function for handling all register operations (may be required by used parsing functions)
	\param exceptionCallback Callback function for handling slave exceptions (optional)
	\param allocator Memory allocator to be used (see \ref modbusDefaultAllocator) (required)
	\param functions Pointer to array of supported function handlers (required).
		The lifetime of this array must not be shorter than the lifetime of the slave.
	\param functionCount Number of function handlers in the array (required)
	\returns MODBUS_NO_ERROR() on success

	\warning This function must not be called on an already initialized ModbusSlave struct.
	\see modbusDefaultAllocator()
	\see modbusSlaveDefaultFunctions
*/
LIGHTMODBUS_RET_ERROR modbusSlaveInit(
	ModbusSlave *status,
	ModbusRegisterCallback registerCallback,
	ModbusSlaveExceptionCallback exceptionCallback,
	ModbusAllocator allocator,
	const ModbusSlaveFunctionHandler *functions,
	uint8_t functionCount)
{
	status->functions = functions;
	status->functionCount = functionCount;
	status->registerCallback = registerCallback;
	status->exceptionCallback = exceptionCallback;
	status->context = NULL;

	return modbusBufferInit(&status->response, allocator);
}

/**
	\brief Frees memory allocated in the ModbusSlave struct
*/
void modbusSlaveDestroy(ModbusSlave *status)
{
	modbusBufferDestroy(&status->response, modbusSlaveGetUserPointer(status));
}

/**
	\brief Builds an exception response frame
	\param function function that reported the exception
	\param code Modbus exception code
	\returns MODBUS_GENERAL_ERROR(ALLOC) on memory allocation failure
	\returns MODBUS_NO_ERROR() on success
	\note If set, `exceptionCallback` from ModbusSlave is called, even if the
		response frame is going to be discarded (when the request was broadcast).

	\warning This function expects ModbusSlave::response::pduOffset and
	ModbusSlave::response::padding to be set properly! If you're looking for a 
	function to manually build an exception please use modbusBuildExceptionPDU(),
	modbusBuildExceptionRTU() or modbusBuildExceptionTCP()
*/
LIGHTMODBUS_RET_ERROR modbusBuildException(
	ModbusSlave *status,
	uint8_t function,
	ModbusExceptionCode code)
{
	// Call the exception callback
	if (status->exceptionCallback)
		status->exceptionCallback(status, function, code);

	if (modbusSlaveAllocateResponse(status, 2))
		return MODBUS_GENERAL_ERROR(ALLOC);

	status->response.pdu[0] = function | 0x80;
	status->response.pdu[1] = code;

	return MODBUS_NO_ERROR();
}

/**
	\brief Builds an exception PDU
	\param function function that reported the exception
	\param code Modbus exception code
	\returns MODBUS_GENERAL_ERROR(ALLOC) on memory allocation failure
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusBuildExceptionPDU(
	ModbusSlave *status,
	uint8_t function,
	ModbusExceptionCode code)
{
	status->response.pduOffset = 0;
	status->response.padding = 0;
	
	ModbusErrorInfo err = modbusBuildException(status, function, code);
	if (!modbusIsOk(err))
		return err;

	return MODBUS_NO_ERROR();
}

/**
	\brief Builds a Modbus RTU exception
	\param address slave address to be reported in the excetion
	\param function function that reported the exception
	\param code Modbus exception code
	\returns MODBUS_GENERAL_ERROR(ADDRESS) if address is 0
	\returns MODBUS_GENERAL_ERROR(ALLOC) on memory allocation failure
	\returns MODBUS_GENERAL_ERROR(LENGTH) if the allocated response has invalid length
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusBuildExceptionRTU(
	ModbusSlave *status,
	uint8_t address,
	uint8_t function,
	ModbusExceptionCode code)
{
	if (address == 0)
		return MODBUS_GENERAL_ERROR(ADDRESS);

	status->response.pduOffset = MODBUS_RTU_PDU_OFFSET;
	status->response.padding = MODBUS_RTU_ADU_PADDING;
	
	ModbusErrorInfo errinfo = modbusBuildException(status, function, code);
	
	if (!modbusIsOk(errinfo))
		return errinfo;

	ModbusError err = modbusPackRTU(
		&status->response.data[0],
		status->response.length,
		address);
		
	// There's literally no reason for modbusPackRTU()
	// to return MODBUS_ERROR_LENGTH here
	(void) err;

	return MODBUS_NO_ERROR();
}

/**
	\brief Builds a Modbus TCP exception
	\param transactionID transaction ID 
	\param unitID unit ID to be reported in the exception
	\param function function that reported the exception
	\param code Modbus exception code
	\returns MODBUS_GENERAL_ERROR(ALLOC) on memory allocation failure
	\returns MODBUS_GENERAL_ERROR(LENGTH) if the allocated response has invalid length
	\returns MODBUS_NO_ERROR() on success
*/
LIGHTMODBUS_RET_ERROR modbusBuildExceptionTCP(
	ModbusSlave *status,
	uint16_t transactionID,
	uint8_t unitID,
	uint8_t function,
	ModbusExceptionCode code)
{
	status->response.pduOffset = MODBUS_TCP_PDU_OFFSET;
	status->response.padding = MODBUS_TCP_ADU_PADDING;
	
	ModbusErrorInfo errinfo = modbusBuildException(status, function, code);
	
	if (!modbusIsOk(errinfo))
		return errinfo;

	ModbusError err = modbusPackTCP(
		&status->response.data[0],
		status->response.length,
		transactionID,
		unitID);

	// There's literally no reason for modbusPackTCP()
	// to return MODBUS_ERROR_LENGTH here
	(void) err;

	return MODBUS_NO_ERROR();
}

/**
	\brief Parses provided PDU and generates response honorinng `pduOffset` and `padding`
		set in ModbusSlave during response generation.
	\param request pointer to the PDU data
	\param requestLength length of the PDU (valid range: 1 - 253)
	\returns Any errors from parsing functions

	\warning This function expects ModbusSlave::response::pduOffset and
	ModbusSlave::response::padding to be set properly! If you're looking for a 
	function to parse PDU and generate a PDU response, please use modbusParseRequestPDU() instead.

	\warning The response frame can only be accessed if modbusIsOk() called 
		on the return value of this function evaluates to true.
*/
LIGHTMODBUS_RET_ERROR modbusParseRequest(ModbusSlave *status, const uint8_t *request, uint8_t requestLength)
{
	uint8_t function = request[0];

	// Look for matching function
	for (uint16_t i = 0; i < status->functionCount; i++)
		if (function == status->functions[i].id)
			return status->functions[i].ptr(status, function, &request[0], requestLength);

	// No match found
	return modbusBuildException(status, function, MODBUS_EXCEP_ILLEGAL_FUNCTION);
}

/**
	\brief Parses provided PDU and generates PDU for the response frame
	\param request pointer to the PDU data
	\param requestLength length of the PDU (valid range: 1 - 253)
	\returns MODBUS_REQUEST_ERROR(LENGTH) if length of the frame is invalid
	\returns Any errors from parsing functions

	\warning The response frame can only be accessed if modbusIsOk() called 
		on the return value from this function evaluates to true.

	\warning The `requestLength` argument is  of type `uint8_t` and not `uint16_t`
		as in case of Modbus RTU and TCP.
*/
LIGHTMODBUS_RET_ERROR modbusParseRequestPDU(ModbusSlave *status, const uint8_t *request, uint8_t requestLength)
{
	// Check length
	if (!requestLength || requestLength > MODBUS_PDU_MAX)
		return MODBUS_REQUEST_ERROR(LENGTH);

	modbusBufferModePDU(&status->response);
	return modbusParseRequest(status, request, requestLength);
}

/**
	\brief Parses provided Modbus RTU request frame and generates a Modbus RTU response
	\param slaveAddress ID of the slave to match with the request
	\param request pointer to a Modbus RTU frame
	\param requestLength length of the frame (valid range: 4 - 256)
	\returns MODBUS_REQUEST_ERROR(LENGTH) if length of the frame is invalid
	\returns MODBUS_REQUEST_ERROR(CRC) if CRC is invalid
	\returns MODBUS_REQUEST_ERROR(ADDRESS) if the request is meant for other slave
	\returns MODBUS_GENERAL_ERROR(LENGTH) if the resulting response frame has invalid length
	\returns Any errors from parsing functions

	\warning The response frame can only be accessed if modbusIsOk() called 
		on the return value of this function evaluates to true.
*/
LIGHTMODBUS_RET_ERROR modbusParseRequestRTU(ModbusSlave *status, uint8_t slaveAddress, const uint8_t *request, uint16_t requestLength)
{
	// Unpack the request
	const uint8_t *pdu;
	uint16_t pduLength;
	uint8_t requestAddress;
	ModbusError err = modbusUnpackRTU(
		request,
		requestLength,
		1,
		&pdu,
		&pduLength,
		&requestAddress
	);

	if (err != MODBUS_OK)
		return MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_REQUEST, err);

	// Verify if the frame is meant for us
	if (requestAddress != 0 && requestAddress != slaveAddress)
		return MODBUS_REQUEST_ERROR(ADDRESS);

	// Parse the request
	ModbusErrorInfo errinfo;
	modbusBufferModeRTU(&status->response);
	if (!modbusIsOk(errinfo = modbusParseRequest(status, pdu, pduLength)))
		return errinfo;
	
	if (status->response.length)
	{
		// Discard any response frames if the request
		// was broadcast
		if (requestAddress == 0)
		{
			modbusSlaveFreeResponse(status);
			return MODBUS_NO_ERROR();
		}

		// Pack the response frame
		err = modbusPackRTU(
			&status->response.data[0],
			status->response.length,
			slaveAddress);

		if (err != MODBUS_OK)
			return MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_GENERAL, err);
	}
	
	return MODBUS_NO_ERROR();
}

/**
	\brief Parses provided Modbus TCP request frame and generates a Modbus TCP response
	\param request pointer to a Modbus TCP frame
	\param requestLength length of the frame (valid range: 8 - 260)
	\returns MODBUS_REQUEST_ERROR(LENGTH) if length of the frame is invalid or different from the declared one
	\returns MODBUS_REQUEST_ERROR(BAD_PROTOCOL) if the frame is not a Modbus TCP message
	\returns MODBUS_GENERAL_ERROR(LENGTH) if the resulting response frame has invalid length
	\returns Any errors from parsing functions

	\warning The response frame can only be accessed if modbusIsOk() called 
		on the return value of this function evaluates to true.
*/
LIGHTMODBUS_RET_ERROR modbusParseRequestTCP(ModbusSlave *status, const uint8_t *request, uint16_t requestLength)
{
	// Unpack the request
	const uint8_t *pdu;
	uint16_t pduLength;
	uint16_t transactionID;
	uint8_t unitID;
	ModbusError err = modbusUnpackTCP(
		request,
		requestLength,
		&pdu,
		&pduLength,
		&transactionID,
		&unitID
	);

	if (err != MODBUS_OK)
		return MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_REQUEST, err);
	
	// Parse the request
	ModbusErrorInfo errinfo;
	modbusBufferModeTCP(&status->response);
	if (!modbusIsOk(errinfo = modbusParseRequest(status, pdu, pduLength)))
		return errinfo;

	// Write MBAP header
	if (status->response.length)
	{
		// Pack the resonse
		err = modbusPackTCP(
			&status->response.data[0],
			status->response.length,
			transactionID,
			unitID
		);

		if (err != MODBUS_OK)
			return MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_GENERAL, err);
	}

	return MODBUS_NO_ERROR();
}

#endif
