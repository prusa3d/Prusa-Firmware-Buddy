#ifndef LIGHTMODBUS_DEBUG_H
#define LIGHTMODBUS_DEBUG_H

#include "base.h"
#include "slave.h"

/**
	\file debug.h
	\brief Debug utilities (header)
*/

const char *modbusErrorStr(ModbusError err);
const char *modbusErrorSourceStr(uint8_t src);
const char *modbusExceptionCodeStr(ModbusExceptionCode code);
const char *modbusDataTypeStr(ModbusDataType type);
const char *modbusRegisterQueryStr(ModbusRegisterQuery query);

#endif
