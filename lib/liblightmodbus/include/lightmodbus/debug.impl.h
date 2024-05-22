#ifndef LIGHTMODBUS_DEBUG_IMPL_H
#define LIGHTMODBUS_DEBUG_IMPL_H

#include "debug.h"

/**
	\file debug.impl.h
	\brief Debug utilities (implementation)
*/

/**
	\def ESTR(x)
	\brief Defines a field in array holding name of the enum value
*/
#define ESTR(x) [(int)(x)] = #x

/**
	\def ECASE(x)
	\brief Defines a case halding enum value, returning the name of the enum value
*/
#define ECASE(x) case (int)(x): return #x; break;

/**
	\brief Returns a string containing the name of the ModbusError value
*/
const char *modbusErrorStr(ModbusError err)
{
	switch (err)
	{
		ECASE(MODBUS_OK);
		ECASE(MODBUS_ERROR_LENGTH);
		ECASE(MODBUS_ERROR_ALLOC);
		ECASE(MODBUS_ERROR_FUNCTION);
		ECASE(MODBUS_ERROR_COUNT);
		ECASE(MODBUS_ERROR_INDEX);
		ECASE(MODBUS_ERROR_VALUE);
		ECASE(MODBUS_ERROR_RANGE);
		ECASE(MODBUS_ERROR_CRC);
		ECASE(MODBUS_ERROR_BAD_PROTOCOL);
		ECASE(MODBUS_ERROR_BAD_TRANSACTION);
		ECASE(MODBUS_ERROR_ADDRESS);
		ECASE(MODBUS_ERROR_OTHER);

		default: return "[invalid ModbusError]";
	};
}

/**
	\brief Returns a string containing the name error source
*/
const char *modbusErrorSourceStr(uint8_t src)
{
	switch (src)
	{
		ECASE(MODBUS_ERROR_SOURCE_GENERAL);
		ECASE(MODBUS_ERROR_SOURCE_REQUEST);
		ECASE(MODBUS_ERROR_SOURCE_RESPONSE);
		ECASE(MODBUS_ERROR_SOURCE_RESERVED);

		default: return "[invalid ModbusErrorInfo source]";
	};
}

/**
	\brief Returns a string containing the name of the ModbusExceptionCode enum value
*/
const char *modbusExceptionCodeStr(ModbusExceptionCode code)
{
	switch (code)
	{
		ECASE(MODBUS_EXCEP_NONE);
		ECASE(MODBUS_EXCEP_ILLEGAL_FUNCTION);
		ECASE(MODBUS_EXCEP_ILLEGAL_ADDRESS);
		ECASE(MODBUS_EXCEP_ILLEGAL_VALUE);
		ECASE(MODBUS_EXCEP_SLAVE_FAILURE);
		ECASE(MODBUS_EXCEP_ACK);
		ECASE(MODBUS_EXCEP_NACK);

		default: return "[invalid ModbusExceptionCode]";
	};
}

/**
	\brief Returns a string containing the name of the ModbusDataType enum value
*/
const char *modbusDataTypeStr(ModbusDataType type)
{
	switch (type)
	{
		ECASE(MODBUS_HOLDING_REGISTER);
		ECASE(MODBUS_INPUT_REGISTER);
		ECASE(MODBUS_COIL);
		ECASE(MODBUS_DISCRETE_INPUT);
		
		default: return "[invalid ModbusDataType]";
	}
}

/**
	\brief Returns a string containing the name of the ModbusRegisterQuery enum value
*/
const char *modbusRegisterQueryStr(ModbusRegisterQuery query)
{
	switch (query)
	{
		ECASE(MODBUS_REGQ_R_CHECK);
		ECASE(MODBUS_REGQ_W_CHECK);
		ECASE(MODBUS_REGQ_R);
		ECASE(MODBUS_REGQ_W);
		
		default: return "[invalid ModbusRegisterQuery]";
	}
}

#undef ESTR
#undef ECASE

#endif
