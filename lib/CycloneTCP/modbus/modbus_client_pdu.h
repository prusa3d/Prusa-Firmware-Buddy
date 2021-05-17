/**
 * @file modbus_client_pdu.h
 * @brief Modbus PDU formatting and parsing
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2021 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

#ifndef _MODBUS_CLIENT_PDU_H
#define _MODBUS_CLIENT_PDU_H

//Dependencies
#include "core/net.h"
#include "modbus/modbus_client.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Modbus/TCP client related functions
error_t modbusClientFormatReadCoilsReq(ModbusClientContext *context,
   uint16_t address, uint_t quantity);

error_t modbusClientFormatReadDiscreteInputsReq(ModbusClientContext *context,
   uint16_t address, uint_t quantity);

error_t modbusClientFormatReadHoldingRegsReq(ModbusClientContext *context,
   uint16_t address, uint_t quantity);

error_t modbusClientFormatReadInputRegsReq(ModbusClientContext *context,
   uint16_t address, uint_t quantity);

error_t modbusClientFormatWriteSingleCoilReq(ModbusClientContext *context,
   uint16_t address, bool_t value);

error_t modbusClientFormatWriteSingleRegReq(ModbusClientContext *context,
   uint16_t address, uint16_t value);

error_t modbusClientFormatWriteMultipleCoilsReq(ModbusClientContext *context,
   uint16_t address, uint_t quantity, const uint8_t *value);

error_t modbusClientFormatWriteMultipleRegsReq(ModbusClientContext *context,
   uint16_t address, uint_t quantity, const uint16_t *value);

error_t modbusClientFormatMaskWriteRegReq(ModbusClientContext *context,
   uint16_t address, uint16_t andMask, uint16_t orMask);

error_t modbusClientFormatReadWriteMultipleRegsReq(ModbusClientContext *context,
   uint16_t readAddress, uint16_t readQuantity, uint16_t writeAddress,
   uint16_t writeQuantity, const uint16_t *writeValue);

error_t modbusClientParseReadCoilsResp(ModbusClientContext *context,
   uint_t quantity, uint8_t *value);

error_t modbusClientParseReadDiscreteInputsResp(ModbusClientContext *context,
   uint_t quantity, uint8_t *value);

error_t modbusClientParseReadHoldingRegsResp(ModbusClientContext *context,
   uint_t quantity, uint16_t *value);

error_t modbusClientParseReadInputRegsResp(ModbusClientContext *context,
   uint_t quantity, uint16_t *value);

error_t modbusClientParseWriteSingleCoilResp(ModbusClientContext *context,
   uint16_t address, bool_t value);

error_t modbusClientParseWriteSingleRegResp(ModbusClientContext *context,
   uint16_t address, uint16_t value);

error_t modbusClientParseWriteMultipleCoilsResp(ModbusClientContext *context,
   uint16_t address, uint_t quantity);

error_t modbusClientParseWriteMultipleRegsResp(ModbusClientContext *context,
   uint16_t address, uint_t quantity);

error_t modbusClientParseMaskWriteRegResp(ModbusClientContext *context,
   uint16_t address, uint16_t andMask, uint16_t orMask);

error_t modbusClientParseReadWriteMultipleRegsResp(ModbusClientContext *context,
   uint_t readQuantity, uint16_t *readValue);

error_t modbusClientParseExceptionResp(ModbusClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
