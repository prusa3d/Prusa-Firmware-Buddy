/**
 * @file modbus_server_pdu.h
 * @brief Modbus PDU processing
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

#ifndef _MODBUS_SERVER_PDU_H
#define _MODBUS_SERVER_PDU_H

//Dependencies
#include "core/net.h"
#include "modbus/modbus_server.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Modbus/TCP server related functions
error_t modbusServerProcessRequest(ModbusClientConnection *connection);

error_t modbusServerProcessReadCoilsReq(ModbusClientConnection *connection,
   const ModbusReadCoilsReq *request, size_t length);

error_t modbusServerProcessReadDiscreteInputsReq(ModbusClientConnection *connection,
   const ModbusReadDiscreteInputsReq *request, size_t length);

error_t modbusServerProcessReadHoldingRegsReq(ModbusClientConnection *connection,
   const ModbusReadHoldingRegsReq *request, size_t length);

error_t modbusServerProcessReadInputRegsReq(ModbusClientConnection *connection,
   const ModbusReadInputRegsReq *request, size_t length);

error_t modbusServerProcessWriteSingleCoilReq(ModbusClientConnection *connection,
   const ModbusWriteSingleCoilReq *request, size_t length);

error_t modbusServerProcessWriteSingleRegReq(ModbusClientConnection *connection,
   const ModbusWriteSingleRegReq *request, size_t length);

error_t modbusServerProcessWriteMultipleCoilsReq(ModbusClientConnection *connection,
   const ModbusWriteMultipleCoilsReq *request, size_t length);

error_t modbusServerProcessWriteMultipleRegsReq(ModbusClientConnection *connection,
   const ModbusWriteMultipleRegsReq *request, size_t length);

error_t modbusServerProcessMaskWriteRegReq(ModbusClientConnection *connection,
   const ModbusMaskWriteRegReq *request, size_t length);

error_t modbusServerProcessReadWriteMultipleRegsReq(ModbusClientConnection *connection,
   const ModbusReadWriteMultipleRegsReq *request, size_t length);

error_t modbusServerFormatExceptionResp(ModbusClientConnection *connection,
   ModbusFunctionCode functionCode, ModbusExceptionCode exceptionCode);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
