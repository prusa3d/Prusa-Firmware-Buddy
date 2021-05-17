/**
 * @file modbus_client.h
 * @brief Modbus/TCP client
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

#ifndef _MODBUS_CLIENT_H
#define _MODBUS_CLIENT_H

//Dependencies
#include "core/net.h"
#include "modbus/modbus_common.h"

//Modbus/TCP client support
#ifndef MODBUS_CLIENT_SUPPORT
   #define MODBUS_CLIENT_SUPPORT ENABLED
#elif (MODBUS_CLIENT_SUPPORT != ENABLED && MODBUS_CLIENT_SUPPORT != DISABLED)
   #error MODBUS_CLIENT_SUPPORT parameter is not valid
#endif

//Modbus/TCP security
#ifndef MODBUS_CLIENT_TLS_SUPPORT
   #define MODBUS_CLIENT_TLS_SUPPORT DISABLED
#elif (MODBUS_CLIENT_TLS_SUPPORT != ENABLED && MODBUS_CLIENT_TLS_SUPPORT != DISABLED)
   #error MODBUS_CLIENT_TLS_SUPPORT parameter is not valid
#endif

//Default timeout
#ifndef MODBUS_CLIENT_DEFAULT_TIMEOUT
   #define MODBUS_CLIENT_DEFAULT_TIMEOUT 20000
#elif (MODBUS_CLIENT_DEFAULT_TIMEOUT < 0)
   #error MODBUS_CLIENT_DEFAULT_TIMEOUT parameter is not valid
#endif

//TX buffer size for TLS connections
#ifndef MODBUS_CLIENT_TLS_TX_BUFFER_SIZE
   #define MODBUS_CLIENT_TLS_TX_BUFFER_SIZE 2048
#elif (MODBUS_CLIENT_TLS_TX_BUFFER_SIZE < 512)
   #error MODBUS_CLIENT_TLS_TX_BUFFER_SIZE parameter is not valid
#endif

//RX buffer size for TLS connections
#ifndef MODBUS_CLIENT_TLS_RX_BUFFER_SIZE
   #define MODBUS_CLIENT_TLS_RX_BUFFER_SIZE 2048
#elif (MODBUS_CLIENT_TLS_RX_BUFFER_SIZE < 512)
   #error MODBUS_CLIENT_TLS_RX_BUFFER_SIZE parameter is not valid
#endif

//TLS supported?
#if (MODBUS_CLIENT_TLS_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "tls.h"
#endif

//Forward declaration of ModbusClientContext structure
struct _ModbusClientContext;
#define ModbusClientContext struct _ModbusClientContext

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Modbus/TCP client states
 */

typedef enum
{
   MODBUS_CLIENT_STATE_DISCONNECTED  = 0,
   MODBUS_CLIENT_STATE_CONNECTING    = 1,
   MODBUS_CLIENT_STATE_CONNECTED     = 2,
   MODBUS_CLIENT_STATE_SENDING       = 3,
   MODBUS_CLIENT_STATE_RECEIVING     = 4,
   MODBUS_CLIENT_STATE_COMPLETE      = 5,
   MODBUS_CLIENT_STATE_DISCONNECTING = 6
} ModbusClientState;


//TLS supported?
#if (MODBUS_CLIENT_TLS_SUPPORT == ENABLED)

/**
 * @brief TLS initialization callback function
 **/

typedef error_t (*ModbusClientTlsInitCallback)(ModbusClientContext *context,
   TlsContext *tlsContext);

#endif


/**
 * @brief Modbus/TCP client context
 **/

struct _ModbusClientContext
{
   ModbusClientState state;                     ///<Modbus/TCP client state
   NetInterface *interface;                     ///<Underlying network interface
   uint8_t unitId;                              ///<Identifier of the remote slave
   uint16_t transactionId;                      ///<Modbus transaction identifier
   Socket *socket;                              ///<Underlying TCP socket
#if (MODBUS_CLIENT_TLS_SUPPORT == ENABLED)
   TlsContext *tlsContext;                      ///<TLS context
   TlsSessionState tlsSession;                  ///<TLS session state
   ModbusClientTlsInitCallback tlsInitCallback; ///<TLS initialization callback function
#endif
   systime_t timeout;                           ///<Timeout value
   systime_t timestamp;                         ///<Timestamp to manage timeout
   uint8_t requestAdu[MODBUS_MAX_ADU_SIZE];     ///<Request ADU
   size_t requestAduLen;                        ///<Length of the request ADU, in bytes
   size_t requestAduPos;                        ///<Current position in the request ADU
   uint8_t responseAdu[MODBUS_MAX_ADU_SIZE];    ///<Response ADU
   size_t responseAduLen;                       ///<Length of the response ADU, in bytes
   size_t responseAduPos;                       ///<Current position in the response ADU
   ModbusExceptionCode exceptionCode;           ///<Exception code
};


//Modbus/TCP client related functions
error_t modbusClientInit(ModbusClientContext *context);

#if (MODBUS_CLIENT_TLS_SUPPORT == ENABLED)

error_t modbusClientRegisterTlsInitCallback(ModbusClientContext *context,
   ModbusClientTlsInitCallback callback);

#endif

error_t modbusClientSetTimeout(ModbusClientContext *context, systime_t timeout);
error_t modbusClientSetUnitId(ModbusClientContext *context, uint8_t unitId);

error_t modbusClientBindToInterface(ModbusClientContext *context,
   NetInterface *interface);

error_t modbusClientConnect(ModbusClientContext *context,
   const IpAddr *serverIpAddr, uint16_t serverPort);

error_t modbusClientReadCoils(ModbusClientContext *context,
   uint16_t address, uint_t quantity, uint8_t *value);

error_t modbusClientReadDiscreteInputs(ModbusClientContext *context,
   uint8_t address, uint_t quantity, uint8_t *value);

error_t modbusClientReadHoldingRegs(ModbusClientContext *context,
   uint16_t address, uint_t quantity, uint16_t *value);

error_t modbusClientReadInputRegs(ModbusClientContext *context,
   uint16_t address, uint_t quantity, uint16_t *value);

error_t modbusClientWriteSingleCoil(ModbusClientContext *context,
   uint16_t address, bool_t value);

error_t modbusClientWriteSingleReg(ModbusClientContext *context,
   uint16_t address, uint16_t value);

error_t modbusClientWriteMultipleCoils(ModbusClientContext *context,
   uint16_t address, uint_t quantity, const uint8_t *value);

error_t modbusClientWriteMultipleRegs(ModbusClientContext *context,
   uint16_t address, uint_t quantity, const uint16_t *value);

error_t modbusClientMaskWriteReg(ModbusClientContext *context,
   uint16_t address, uint16_t andMask, uint16_t orMask);

error_t modbusClientReadWriteMultipleRegs(ModbusClientContext *context,
   uint16_t readAddress, uint_t readQuantity, uint16_t *readValue,
   uint16_t writeAddress, uint_t writeQuantity, const uint16_t *writeValue);

error_t modbusClientGetExceptionCode(ModbusClientContext *context,
   ModbusExceptionCode *exceptionCode);

error_t modbusClientDisconnect(ModbusClientContext *context);
error_t modbusClientClose(ModbusClientContext *context);

void modbusClientDeinit(ModbusClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
