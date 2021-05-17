/**
 * @file modbus_server_misc.h
 * @brief Helper functions for Modbus/TCP server
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

#ifndef _MODBUS_SERVER_MISC_H
#define _MODBUS_SERVER_MISC_H

//Dependencies
#include "core/net.h"
#include "modbus/modbus_server.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Modbus/TCP server related functions
void modbusServerTick(ModbusServerContext *context);

void modbusServerRegisterConnectionEvents(ModbusClientConnection *connection,
   SocketEventDesc *eventDesc);

void modbusServerProcessConnectionEvents(ModbusClientConnection *connection);

error_t modbusServerParseMbapHeader(ModbusClientConnection *connection);

error_t modbusServerFormatMbapHeader(ModbusClientConnection *connection,
   size_t length);

void *modbusServerGetRequestPdu(ModbusClientConnection *connection,
   size_t *length);

void *modbusServerGetResponsePdu(ModbusClientConnection *connection);

void modbusServerLock(ModbusClientConnection *connection);
void modbusServerUnlock(ModbusClientConnection *connection);

error_t modbusServerReadCoil(ModbusClientConnection *connection,
   uint16_t address, bool_t *state);

error_t modbusServerReadDiscreteInput(ModbusClientConnection *connection,
   uint16_t address, bool_t *state);

error_t modbusServerWriteCoil(ModbusClientConnection *connection,
   uint16_t address, bool_t state, bool_t commit);

error_t modbusServerReadHoldingReg(ModbusClientConnection *connection,
   uint16_t address, uint16_t *value);

error_t modbusServerReadInputReg(ModbusClientConnection *connection,
   uint16_t address, uint16_t *value);

error_t modbusServerWriteReg(ModbusClientConnection *connection,
   uint16_t address, uint16_t value, bool_t commit);

ModbusExceptionCode modbusServerTranslateExceptionCode(error_t status);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
