/**
 * @file modbus_server_security.h
 * @brief Modbus/TCP security layer
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

#ifndef _MODBUS_SERVER_SECURITY_H
#define _MODBUS_SERVER_SECURITY_H

//Dependencies
#include "core/net.h"
#include "modbus/modbus_server.h"

//TLS supported?
#if (MODBUS_SERVER_TLS_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "encoding/asn1.h"
   #include "encoding/oid.h"
   #include "pkix/x509_cert_parse.h"
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Modbus/TCP server related functions
error_t modbusServerParseRoleOid(ModbusClientConnection *connection,
   const uint8_t *data, size_t length);

error_t modbusServerOpenSecureConnection(ModbusServerContext *context,
   ModbusClientConnection *connection);

error_t modbusServerEstablishSecureConnection(ModbusClientConnection *connection);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
