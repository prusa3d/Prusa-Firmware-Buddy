/**
 * @file modbus_debug.h
 * @brief Data logging functions for debugging purpose (Modbus/TCP)
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

#ifndef _MODBUS_DEBUG_H
#define _MODBUS_DEBUG_H

//Dependencies
#include "core/net.h"
#include "modbus/modbus_common.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Modbus/TCP related functions
error_t modbusDumpRequestPdu(const void *pdu, size_t length);
error_t modbusDumpResponsePdu(const void *pdu, size_t length);

error_t modbusDumpReadCoilsReq(const ModbusReadCoilsReq *request,
   size_t length);

error_t modbusDumpReadCoilsResp(const ModbusReadCoilsResp *response,
   size_t length);

error_t modbusDumpReadDiscreteInputsReq(const ModbusReadDiscreteInputsReq *request,
   size_t length);

error_t modbusDumpReadDiscreteInputsResp(const ModbusReadDiscreteInputsResp *response,
   size_t length);

error_t modbusDumpReadHoldingRegsReq(const ModbusReadHoldingRegsReq *request,
   size_t length);

error_t modbusDumpReadHoldingRegsResp(const ModbusReadHoldingRegsResp *response,
   size_t length);

error_t modbusDumpReadInputRegsReq(const ModbusReadInputRegsReq *request,
   size_t length);

error_t modbusDumpReadInputRegsResp(const ModbusReadInputRegsResp *response,
   size_t length);

error_t modbusDumpWriteSingleCoilReq(const ModbusWriteSingleCoilReq *request,
   size_t length);

error_t modbusDumpWriteSingleCoilResp(const ModbusWriteSingleCoilResp *response,
   size_t length);

error_t modbusDumpWriteSingleRegReq(const ModbusWriteSingleRegReq *request,
   size_t length);

error_t modbusDumpWriteSingleRegResp(const ModbusWriteSingleRegResp *response,
   size_t length);

error_t modbusDumpWriteMultipleCoilsReq(const ModbusWriteMultipleCoilsReq *request,
   size_t length);

error_t modbusDumpWriteMultipleCoilsResp(const ModbusWriteMultipleCoilsResp *response,
   size_t length);

error_t modbusDumpWriteMultipleRegsReq(const ModbusWriteMultipleRegsReq *request,
   size_t length);

error_t modbusDumpWriteMultipleRegsResp(const ModbusWriteMultipleRegsResp *response,
   size_t length);

error_t modbusDumpMaskWriteRegReq(const ModbusMaskWriteRegReq *request,
   size_t length);

error_t modbusDumpMaskWriteRegResp(const ModbusMaskWriteRegResp *response,
   size_t length);

error_t modbusDumpReadWriteMultipleRegsReq(const ModbusReadWriteMultipleRegsReq *request,
   size_t length);

error_t modbusDumpReadWriteMultipleRegsResp(const ModbusReadWriteMultipleRegsResp *response,
   size_t length);

error_t modbusDumpExceptionResp(const ModbusExceptionResp *response,
   size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
