#ifndef LIGHTMODBUS_HPP
#define LIGHTMODBUS_HPP
#include <stdexcept>

#ifndef LIGHTMODBUS_DEBUG
#define LIGHTMODBUS_DEBUG // FIXME
#endif

#include "lightmodbus.h"

/**
	\file lightmodbus.hpp
	\brief (Very) experimental liblightmodbus C++ interface

	\warning THIS INTERFACE IS EXPERIMENTAL, NOT TESTED AND MAY CHANGE AT ANY TIME.
*/

/**
	\brief The liblightmodbus C++ namepsace
*/
namespace llm {


/**
	\brief Request error exception class
*/
class RequestError : public std::exception
{
public:
	explicit RequestError(ModbusError err) :
		m_error(err)
	{
	}

	const char *what() const noexcept override
	{
		return modbusErrorStr(m_error);
	}

private:
	ModbusError m_error;
};

/**
	\brief Response error exception class
*/
class ResponseError : public std::exception
{
public:
	explicit ResponseError(ModbusError err) :
		m_error(err)
	{
	}

	const char *what() const noexcept override
	{
		return modbusErrorStr(m_error);
	}

private:
	ModbusError m_error;
};

/**
	\brief General error exception class
*/
class GeneralError : public std::exception
{
public:
	explicit GeneralError(ModbusError err) :
		m_error(err)
	{
	}

	const char *what() const noexcept override
	{
		return modbusErrorStr(m_error);
	}

private:
	ModbusError m_error;
};

/**
	\brief Throws an exception based on ModbusErrorInfo.
	\param err The ModbusErrorInfo to convert to an exception
	\throws GeneralError if `modbusGetGeneralError(err)` is not MODBUS_OK
	\throws RequestError if `modbusGetRequestError(err)` is not MODBUS_OK
	\throws ResponseError if `modbusGetResponseError(err)` is not MODBUS_OK
*/
static inline void throwErrorInfo(ModbusErrorInfo err)
{
	if (modbusGetGeneralError(err) != MODBUS_OK)
		throw GeneralError(modbusGetGeneralError(err));
	else if (modbusGetRequestError(err) != MODBUS_OK)
		throw RequestError(modbusGetRequestError(err));
	else if (modbusGetResponseError(err) != MODBUS_OK)
		throw ResponseError(modbusGetResponseError(err));
}

#ifdef LIGHTMODBUS_SLAVE

/**
	\brief Represents a Modbus slave device
*/
class Slave
{
public:
	Slave(
		ModbusRegisterCallback registerCallback,
		ModbusSlaveExceptionCallback exceptionCallback = nullptr,
		ModbusAllocator allocator = modbusDefaultAllocator,
		ModbusSlaveFunctionHandler *functions = modbusSlaveDefaultFunctions,
		uint16_t functionCount = modbusSlaveDefaultFunctionCount)
	{
		throwErrorInfo(modbusSlaveInit(
			&m_slave,
			registerCallback,
			exceptionCallback,
			allocator,
			functions,
			functionCount));
	}

	~Slave()
	{
		modbusSlaveDestroy(&m_slave);
	}

	// Disable copy (and move for now)
	Slave(const Slave &) = delete;
	Slave(Slave &&) = delete;
	Slave &operator=(const Slave &) = delete;
	Slave &operator=(Slave &&) = delete;

	void parseRequestPDU(const uint8_t *frame, uint16_t length)
	{
		ModbusErrorInfo err = modbusParseRequestPDU(&m_slave, frame, length);
		m_ok = modbusIsOk(err);
		throwErrorInfo(err);
	}

	void parseRequestRTU(uint8_t address, const uint8_t *frame, uint16_t length)
	{
		ModbusErrorInfo err = modbusParseRequestRTU(&m_slave, address, frame, length);
		m_ok = modbusIsOk(err);
		throwErrorInfo(err);
	}

	void parseRequestTCP(const uint8_t *frame, uint16_t length)
	{
		ModbusErrorInfo err = modbusParseRequestTCP(&m_slave, frame, length);
		m_ok = modbusIsOk(err);
		throwErrorInfo(err);
	}

	void buildExceptionPDU(uint8_t function, ModbusExceptionCode code)
	{
		ModbusErrorInfo err = modbusBuildExceptionPDU(&m_slave, function, code);
		m_ok = modbusIsOk(err);
		throwErrorInfo(err);
	}
	
	void buildExceptionRTU(uint8_t address, uint8_t function, ModbusExceptionCode code)
	{
		ModbusErrorInfo err = modbusBuildExceptionRTU(&m_slave, address, function, code);
		m_ok = modbusIsOk(err);
		throwErrorInfo(err);
	}

	void buildExceptionTCP(uint16_t transactionID, uint8_t unitID, uint8_t function, ModbusExceptionCode code)
	{
		ModbusErrorInfo err = modbusBuildExceptionTCP(&m_slave, transactionID, unitID, function, code);
		m_ok = modbusIsOk(err);
		throwErrorInfo(err);
	}

	const uint8_t *getResponse() const
	{
		if (!m_ok) throw GeneralError(MODBUS_ERROR_OTHER);
		return modbusSlaveGetResponse(&m_slave);
	}

	uint16_t getResponseLength() const
	{
		if (!m_ok) throw GeneralError(MODBUS_ERROR_OTHER);
		return modbusSlaveGetResponseLength(&m_slave);
	}

	void freeResponse()
	{
		modbusSlaveFreeResponse(&m_slave);
		m_ok = false;
	}

	void setUserPointer(void *ptr)
	{
		modbusSlaveSetUserPointer(&m_slave, ptr);
	}

	void *getUserPointer() const
	{
		return modbusSlaveGetUserPointer(&m_slave);
	}

protected:
	ModbusSlave m_slave;
	bool m_ok = false;
};
#endif

#ifdef LIGHTMODBUS_MASTER

#define LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_HEADER(f_suffix, ...) \
	void buildRequest##f_suffix##PDU(__VA_ARGS__)

#define LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_BODY(f_suffix, ...) \
	{ \
		ModbusErrorInfo err = modbusBuildRequest##f_suffix##PDU(&m_master, __VA_ARGS__); \
		m_ok = modbusIsOk(err); \
		throwErrorInfo(err); \
	}

#define LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_HEADER(f_suffix, ...) \
	void buildRequest##f_suffix##RTU(uint8_t address, __VA_ARGS__)

#define LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_BODY(f_suffix, ...) \
	{ \
		ModbusErrorInfo err = modbusBuildRequest##f_suffix##RTU(&m_master, address, __VA_ARGS__); \
		m_ok = modbusIsOk(err); \
		throwErrorInfo(err); \
	}

#define LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_HEADER(f_suffix, ...) \
	void buildRequest##f_suffix##TCP(uint16_t transactionID, uint8_t unitID, __VA_ARGS__)

#define LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_BODY(f_suffix, ...) \
	{ \
		ModbusErrorInfo err = modbusBuildRequest##f_suffix##TCP(&m_master, transactionID, unitID, __VA_ARGS__); \
		m_ok = modbusIsOk(err); \
		throwErrorInfo(err); \
	}


/**
	\brief Represents a Modbus master device
*/
class Master
{
public:
	Master(
		ModbusDataCallback dataCallback,
		ModbusMasterExceptionCallback exceptionCallback = nullptr,
		ModbusAllocator allocator = modbusDefaultAllocator,
		ModbusMasterFunctionHandler *functions = modbusMasterDefaultFunctions,
		uint16_t functionCount = modbusMasterDefaultFunctionCount)
	{
		throwErrorInfo(modbusMasterInit(
			&m_master,
			dataCallback,
			exceptionCallback,
			allocator,
			functions,
			functionCount));
	}

	~Master()
	{
		modbusMasterDestroy(&m_master);
	}

	// Disable copy (and move for now)
	Master(const Master &) = delete;
	Master(Master &&) = delete;
	Master &operator=(const Master &) = delete;
	Master &operator=(Master &&) = delete;

	void parseResponsePDU(
		uint8_t address,
		const uint8_t *request,
		uint8_t requestLength,
		const uint8_t *response,
		uint8_t responseLength)
	{
		ModbusErrorInfo err = modbusParseResponsePDU(
			&m_master,
			address,
			request,
			requestLength,
			response,
			responseLength);
		m_ok = modbusIsOk(err);
		throwErrorInfo(err);
	}

	void parseResponseRTU(
		const uint8_t *request,
		uint8_t requestLength,
		const uint8_t *response,
		uint8_t responseLength)
	{
		ModbusErrorInfo err = modbusParseResponseRTU(
			&m_master,
			request,
			requestLength,
			response,
			responseLength);
		m_ok = modbusIsOk(err);
		throwErrorInfo(err);
	}

	void parseResponseTCP(
		const uint8_t *request,
		uint8_t requestLength,
		const uint8_t *response,
		uint8_t responseLength)
	{
		ModbusErrorInfo err = modbusParseResponseTCP(
			&m_master,
			request,
			requestLength,
			response,
			responseLength);
		m_ok = modbusIsOk(err);
		throwErrorInfo(err);
	}

	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_HEADER(01, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_BODY(01, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_HEADER(02, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_BODY(02, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_HEADER(03, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_BODY(03, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_HEADER(04, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_BODY(04, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_HEADER(05, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_BODY(05, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_HEADER(06, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_BODY(06, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_HEADER(15, uint16_t index, uint16_t count, const uint8_t *values)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_BODY(15, index, count, values)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_HEADER(16, uint16_t index, uint16_t count, const uint16_t *values)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_BODY(16, index, count, values)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_HEADER(22, uint16_t index, uint16_t andmask, uint16_t ormask)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_PDU_BODY(22, index, andmask, ormask)

	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_HEADER(01, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_BODY(01, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_HEADER(02, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_BODY(02, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_HEADER(03, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_BODY(03, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_HEADER(04, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_BODY(04, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_HEADER(05, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_BODY(05, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_HEADER(06, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_BODY(06, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_HEADER(15, uint16_t index, uint16_t count, const uint8_t *values)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_BODY(15, index, count, values)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_HEADER(16, uint16_t index, uint16_t count, const uint16_t *values)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_BODY(16, index, count, values)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_HEADER(22, uint16_t index, uint16_t andmask, uint16_t ormask)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_RTU_BODY(22, index, andmask, ormask)

	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_HEADER(01, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_BODY(01, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_HEADER(02, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_BODY(02, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_HEADER(03, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_BODY(03, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_HEADER(04, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_BODY(04, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_HEADER(05, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_BODY(05, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_HEADER(06, uint16_t index, uint16_t count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_BODY(06, index, count)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_HEADER(15, uint16_t index, uint16_t count, const uint8_t *values)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_BODY(15, index, count, values)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_HEADER(16, uint16_t index, uint16_t count, const uint16_t *values)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_BODY(16, index, count, values)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_HEADER(22, uint16_t index, uint16_t andmask, uint16_t ormask)
	LIGHTMODBUS_DEFINE_MEMBER_BUILD_TCP_BODY(22, index, andmask, ormask)

	const uint8_t *getRequest() const
	{
		if (!m_ok) throw GeneralError(MODBUS_ERROR_OTHER);
		return modbusMasterGetRequest(&m_master);
	}

	uint16_t getRequestLength() const
	{
		if (!m_ok) throw GeneralError(MODBUS_ERROR_OTHER);
		return modbusMasterGetRequestLength(&m_master);
	}

	void freeRequest()
	{
		modbusMasterFreeRequest(&m_master);
		m_ok = false;
	}

	void setUserPointer(void *ptr)
	{
		modbusMasterSetUserPointer(&m_master, ptr);
	}

	void *getUserPointer() const
	{
		return modbusMasterGetUserPointer(&m_master);
	}
protected:
	ModbusMaster m_master;
	bool m_ok = false;
};
#endif

}

#endif
