/**
 * @file modbus_client.c
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

//Switch to the appropriate trace level
#define TRACE_LEVEL MODBUS_TRACE_LEVEL

//Dependencies
#include "modbus/modbus_client.h"
#include "modbus/modbus_client_pdu.h"
#include "modbus/modbus_client_transport.h"
#include "modbus/modbus_client_misc.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MODBUS_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Initialize Modbus/TCP client context
 * @param[in] context Pointer to the Modbus/TCP client context
 * @return Error code
 **/

error_t modbusClientInit(ModbusClientContext *context)
{
#if (MODBUS_CLIENT_TLS_SUPPORT == ENABLED)
   error_t error;
#endif

   //Make sure the Modbus/TCP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Clear Modbus/TCP client context
   osMemset(context, 0, sizeof(ModbusClientContext));

#if (MODBUS_CLIENT_TLS_SUPPORT == ENABLED)
   //Initialize TLS session state
   error = tlsInitSessionState(&context->tlsSession);
   //Any error to report?
   if(error)
      return error;
#endif

   //Initialize Modbus/TCP client state
   context->state = MODBUS_CLIENT_STATE_DISCONNECTED;

   //Default timeout
   context->timeout = MODBUS_CLIENT_DEFAULT_TIMEOUT;
   //Default unit identifier
   context->unitId = MODBUS_DEFAULT_UNIT_ID;

   //The transaction identifier is used to uniquely identify the matching
   //requests and responses
   context->transactionId = (uint16_t) netGetRand();

   //Successful initialization
   return NO_ERROR;
}


#if (MODBUS_CLIENT_TLS_SUPPORT == ENABLED)

/**
 * @brief Register TLS initialization callback function
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] callback TLS initialization callback function
 * @return Error code
 **/

error_t modbusClientRegisterTlsInitCallback(ModbusClientContext *context,
   ModbusClientTlsInitCallback callback)
{
   //Make sure the Modbus/TCP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save callback function
   context->tlsInitCallback = callback;

   //Successful processing
   return NO_ERROR;
}

#endif


/**
 * @brief Set timeout value for blocking operations
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] timeout Timeout value, in milliseconds
 * @return Error code
 **/

error_t modbusClientSetTimeout(ModbusClientContext *context, systime_t timeout)
{
   //Make sure the Modbus/TCP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save timeout value
   context->timeout = timeout;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set unit identifier
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] unitId Identifier of the remote slave
 * @return Error code
 **/

error_t modbusClientSetUnitId(ModbusClientContext *context, uint8_t unitId)
{
   //Make sure the Modbus/TCP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save unit identifier
   context->unitId = unitId;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Bind the Modbus/TCP client to a particular network interface
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] interface Network interface to be used
 * @return Error code
 **/

error_t modbusClientBindToInterface(ModbusClientContext *context,
   NetInterface *interface)
{
   //Make sure the Modbus/TCP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Explicitly associate the Modbus/TCP client with the specified interface
   context->interface = interface;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Establish connection with the Modbus/TCP server
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] serverIpAddr IP address of the server to connect to
 * @param[in] serverPort TCP port number that will be used
 * @return Error code
 **/

error_t modbusClientConnect(ModbusClientContext *context,
   const IpAddr *serverIpAddr, uint16_t serverPort)
{
   error_t error;

   //Check parameters
   if(context == NULL || serverIpAddr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Establish connection with the Modbus/TCP server
   while(!error)
   {
      //Check current state
      if(context->state == MODBUS_CLIENT_STATE_DISCONNECTED)
      {
         //Open network connection
         error = modbusClientOpenConnection(context);

         //Check status code
         if(!error)
         {
            //Save current time
            context->timestamp = osGetSystemTime();
            //Update Modbus/TCP client state
            context->state = MODBUS_CLIENT_STATE_CONNECTING;
         }
      }
      else if(context->state == MODBUS_CLIENT_STATE_CONNECTING)
      {
         //Establish network connection
         error = modbusClientEstablishConnection(context, serverIpAddr,
            serverPort);

         //Check status code
         if(error == NO_ERROR)
         {
            //Update Modbus/TCP client state
            context->state = MODBUS_CLIENT_STATE_CONNECTED;
         }
         else if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
         {
            //Check whether the timeout has elapsed
            error = modbusClientCheckTimeout(context);
         }
         else
         {
            //A communication error has occurred
         }
      }
      else if(context->state == MODBUS_CLIENT_STATE_CONNECTED)
      {
         //The Modbus/TCP client is connected
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }
   }

   //Failed to establish connection with the Modbus/TCP server?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Clean up side effects
      modbusClientCloseConnection(context);
      //Update Modbus/TCP client state
      context->state = MODBUS_CLIENT_STATE_DISCONNECTED;
   }

   //Return status code
   return error;
}


/**
 * @brief Read coils
 *
 * This function code is used to read from 1 to 2000 contiguous status of
 * coils in a remote device. The request specifies the starting address and
 * the number of coils
 *
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the first coil
 * @param[in] quantity Number of coils
 * @param[out] value Value of the discrete outputs
 * @return Error code
 **/

error_t modbusClientReadCoils(ModbusClientContext *context,
   uint16_t address, uint_t quantity, uint8_t *value)
{
   error_t error;

   //Check parameters
   if(context == NULL || value == NULL)
      return ERROR_INVALID_PARAMETER;

   //The number of coils must be in range 1 to 2000
   if(quantity < 1 || quantity > 2000)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Perform Modbus request/response transaction
   while(!error)
   {
      //Check current state
      if(context->state == MODBUS_CLIENT_STATE_CONNECTED)
      {
         //Format request
         error = modbusClientFormatReadCoilsReq(context, address,
            quantity);
      }
      else if(context->state == MODBUS_CLIENT_STATE_SENDING ||
         context->state == MODBUS_CLIENT_STATE_RECEIVING)
      {
         //Send Modbus request and wait for a matching response
         error = modbusClientTransaction(context);
      }
      else if(context->state == MODBUS_CLIENT_STATE_COMPLETE)
      {
         //Parse response
         error = modbusClientParseReadCoilsResp(context, quantity,
            value);

         //The Modbus transaction is complete
         context->state = MODBUS_CLIENT_STATE_CONNECTED;
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Read discrete inputs
 *
 * This function code is used to read from 1 to 2000 contiguous status of
 * discrete inputs in a remote device. The request specifies the starting
 * address and the number of inputs
 *
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the first input
 * @param[in] quantity Number of inputs
 * @param[out] value Value of the discrete inputs
 * @return Error code
 **/

error_t modbusClientReadDiscreteInputs(ModbusClientContext *context,
   uint8_t address, uint_t quantity, uint8_t *value)
{
   error_t error;

   //Check parameters
   if(context == NULL || value == NULL)
      return ERROR_INVALID_PARAMETER;

   //The number of discrete inputs must be in range 1 to 2000
   if(quantity < 1 || quantity > 2000)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Perform Modbus request/response transaction
   while(!error)
   {
      //Check current state
      if(context->state == MODBUS_CLIENT_STATE_CONNECTED)
      {
         //Format request
         error = modbusClientFormatReadDiscreteInputsReq(context, address,
            quantity);
      }
      else if(context->state == MODBUS_CLIENT_STATE_SENDING ||
         context->state == MODBUS_CLIENT_STATE_RECEIVING)
      {
         //Send Modbus request and wait for a matching response
         error = modbusClientTransaction(context);
      }
      else if(context->state == MODBUS_CLIENT_STATE_COMPLETE)
      {
         //Parse response
         error = modbusClientParseReadDiscreteInputsResp(context, quantity,
            value);

         //The Modbus transaction is complete
         context->state = MODBUS_CLIENT_STATE_CONNECTED;
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Read holding registers
 *
 * This function code is used to read the contents of a contiguous block of
 * holding registers in a remote device. The request specifies the starting
 * register address and the number of registers
 *
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Starting register address
 * @param[in] quantity Number of registers
 * @param[out] value Value of the holding registers
 * @return Error code
 **/

error_t modbusClientReadHoldingRegs(ModbusClientContext *context,
   uint16_t address, uint_t quantity, uint16_t *value)
{
   error_t error;

   //Check parameters
   if(context == NULL || value == NULL)
      return ERROR_INVALID_PARAMETER;

   //The number of registers must be in range 1 to 125
   if(quantity < 1 || quantity > 125)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Perform Modbus request/response transaction
   while(!error)
   {
      //Check current state
      if(context->state == MODBUS_CLIENT_STATE_CONNECTED)
      {
         //Format request
         error = modbusClientFormatReadHoldingRegsReq(context, address,
            quantity);
      }
      else if(context->state == MODBUS_CLIENT_STATE_SENDING ||
         context->state == MODBUS_CLIENT_STATE_RECEIVING)
      {
         //Send Modbus request and wait for a matching response
         error = modbusClientTransaction(context);
      }
      else if(context->state == MODBUS_CLIENT_STATE_COMPLETE)
      {
         //Parse response
         error = modbusClientParseReadHoldingRegsResp(context, quantity,
            value);

         //The Modbus transaction is complete
         context->state = MODBUS_CLIENT_STATE_CONNECTED;
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Read input registers
 *
 * This function code is used to read from 1 to 125 contiguous input registers
 * in a remote device. The request specifies the starting register address and
 * the number of registers
 *
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Starting register address
 * @param[in] quantity Number of registers
 * @param[out] value Value of the input registers
 * @return Error code
 **/

error_t modbusClientReadInputRegs(ModbusClientContext *context,
   uint16_t address, uint_t quantity, uint16_t *value)
{
   error_t error;

   //Check parameters
   if(context == NULL || value == NULL)
      return ERROR_INVALID_PARAMETER;

   //The number of registers must be in range 1 to 125
   if(quantity < 1 || quantity > 125)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Perform Modbus request/response transaction
   while(!error)
   {
      //Check current state
      if(context->state == MODBUS_CLIENT_STATE_CONNECTED)
      {
         //Format request
         error = modbusClientFormatReadInputRegsReq(context, address,
            quantity);
      }
      else if(context->state == MODBUS_CLIENT_STATE_SENDING ||
         context->state == MODBUS_CLIENT_STATE_RECEIVING)
      {
         //Send Modbus request and wait for a matching response
         error = modbusClientTransaction(context);
      }
      else if(context->state == MODBUS_CLIENT_STATE_COMPLETE)
      {
         //Parse response
         error = modbusClientParseReadInputRegsResp(context, quantity,
            value);

         //The Modbus transaction is complete
         context->state = MODBUS_CLIENT_STATE_CONNECTED;
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Write single coil
 *
 * This function code is used to write a single output to either ON or OFF in
 * a remote device. The request specifies the address of the coil to be forced
 * and the requested ON/OFF state
 *
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the coil to be forced
 * @param[in] value Value of the discrete output
 * @return Error code
 **/

error_t modbusClientWriteSingleCoil(ModbusClientContext *context,
   uint16_t address, bool_t value)
{
   error_t error;

   //Make sure the Modbus/TCP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Perform Modbus request/response transaction
   while(!error)
   {
      //Check current state
      if(context->state == MODBUS_CLIENT_STATE_CONNECTED)
      {
         //Format request
         error = modbusClientFormatWriteSingleCoilReq(context, address, value);
      }
      else if(context->state == MODBUS_CLIENT_STATE_SENDING ||
         context->state == MODBUS_CLIENT_STATE_RECEIVING)
      {
         //Send Modbus request and wait for a matching response
         error = modbusClientTransaction(context);
      }
      else if(context->state == MODBUS_CLIENT_STATE_COMPLETE)
      {
         //Parse response
         error = modbusClientParseWriteSingleCoilResp(context, address, value);

         //The Modbus transaction is complete
         context->state = MODBUS_CLIENT_STATE_CONNECTED;
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Write single register
 *
 * This function code is used to write a single holding register in a remote
 * device. The request specifies the address of the register to be written and
 * the register value
 *
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the register to be written
 * @param[in] value Register value
 * @return Error code
 **/

error_t modbusClientWriteSingleReg(ModbusClientContext *context,
   uint16_t address, uint16_t value)
{
   error_t error;

   //Make sure the Modbus/TCP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Perform Modbus request/response transaction
   while(!error)
   {
      //Check current state
      if(context->state == MODBUS_CLIENT_STATE_CONNECTED)
      {
         //Format request
         error = modbusClientFormatWriteSingleRegReq(context, address, value);
      }
      else if(context->state == MODBUS_CLIENT_STATE_SENDING ||
         context->state == MODBUS_CLIENT_STATE_RECEIVING)
      {
         //Send Modbus request and wait for a matching response
         error = modbusClientTransaction(context);
      }
      else if(context->state == MODBUS_CLIENT_STATE_COMPLETE)
      {
         //Parse response
         error = modbusClientParseWriteSingleRegResp(context, address, value);

         //The Modbus transaction is complete
         context->state = MODBUS_CLIENT_STATE_CONNECTED;
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Write multiple coils
 *
 * This function code is used to force each coil in a sequence of coils to
 * either ON or OFF in a remote device. The request specifies the starting
 * address, the number of outputs and the requested ON/OFF states
 *
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the first coil to be forced
 * @param[in] quantity Number of coils
 * @param[in] value Value of the discrete outputs
 * @return Error code
 **/

error_t modbusClientWriteMultipleCoils(ModbusClientContext *context,
   uint16_t address, uint_t quantity, const uint8_t *value)
{
   error_t error;

   //Check parameters
   if(context == NULL || value == NULL)
      return ERROR_INVALID_PARAMETER;

   //The number of coils must be in range 1 to 1968
   if(quantity < 1 || quantity > 1968)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Perform Modbus request/response transaction
   while(!error)
   {
      //Check current state
      if(context->state == MODBUS_CLIENT_STATE_CONNECTED)
      {
         //Format request
         error = modbusClientFormatWriteMultipleCoilsReq(context, address,
            quantity, value);
      }
      else if(context->state == MODBUS_CLIENT_STATE_SENDING ||
         context->state == MODBUS_CLIENT_STATE_RECEIVING)
      {
         //Send Modbus request and wait for a matching response
         error = modbusClientTransaction(context);
      }
      else if(context->state == MODBUS_CLIENT_STATE_COMPLETE)
      {
         //Parse response
         error = modbusClientParseWriteMultipleCoilsResp(context, address,
            quantity);

         //The Modbus transaction is complete
         context->state = MODBUS_CLIENT_STATE_CONNECTED;
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Write multiple registers
 *
 * This function code is used to write a block of contiguous registers (1 to
 * 123 registers) in a remote device. The request specifies the starting
 * address, the number of registers and the requested written values
 *
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Starting register address
 * @param[in] quantity Number of registers
 * @param[in] value Value of the holding registers
 * @return Error code
 **/

error_t modbusClientWriteMultipleRegs(ModbusClientContext *context,
   uint16_t address, uint_t quantity, const uint16_t *value)
{
   error_t error;

   //Check parameters
   if(context == NULL || value == NULL)
      return ERROR_INVALID_PARAMETER;

   //The number of registers must be in range 1 to 123
   if(quantity < 1 || quantity > 123)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Perform Modbus request/response transaction
   while(!error)
   {
      //Check current state
      if(context->state == MODBUS_CLIENT_STATE_CONNECTED)
      {
         //Format request
         error = modbusClientFormatWriteMultipleRegsReq(context, address,
            quantity, value);
      }
      else if(context->state == MODBUS_CLIENT_STATE_SENDING ||
         context->state == MODBUS_CLIENT_STATE_RECEIVING)
      {
         //Send Modbus request and wait for a matching response
         error = modbusClientTransaction(context);
      }
      else if(context->state == MODBUS_CLIENT_STATE_COMPLETE)
      {
         //Parse response
         error = modbusClientParseWriteMultipleRegsResp(context, address,
            quantity);

         //The Modbus transaction is complete
         context->state = MODBUS_CLIENT_STATE_CONNECTED;
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Apply AND/OR bitmask to a register
 *
 * This function code is used to modify the contents of a specified holding
 * register using a combination of an AND mask, an OR mask, and the register's
 * current contents. The function can be used to set or clear individual bits
 * in the register
 *
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the holding register
 * @param[in] andMask AND bitmask
 * @param[in] orMask OR bitmask
 * @return Error code
 **/

error_t modbusClientMaskWriteReg(ModbusClientContext *context,
   uint16_t address, uint16_t andMask, uint16_t orMask)
{
   error_t error;

   //Make sure the Modbus/TCP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Perform Modbus request/response transaction
   while(!error)
   {
      //Check current state
      if(context->state == MODBUS_CLIENT_STATE_CONNECTED)
      {
         //Format request
         error = modbusClientFormatMaskWriteRegReq(context, address,
            andMask, orMask);
      }
      else if(context->state == MODBUS_CLIENT_STATE_SENDING ||
         context->state == MODBUS_CLIENT_STATE_RECEIVING)
      {
         //Send Modbus request and wait for a matching response
         error = modbusClientTransaction(context);
      }
      else if(context->state == MODBUS_CLIENT_STATE_COMPLETE)
      {
         //Parse response
         error = modbusClientParseMaskWriteRegResp(context, address,
            andMask, orMask);

         //The Modbus transaction is complete
         context->state = MODBUS_CLIENT_STATE_CONNECTED;
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Read/write multiple registers
 *
 * This function code performs a combination of one read operation and one
 * write operation in a single Modbus transaction. The write operation is
 * performed before the read
 *
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] readAddress Address of the first holding registers to be read
 * @param[in] readQuantity Number of holding registers to be read
 * @param[out] readValue Value of the holding registers (read operation)
 * @param[in] writeAddress Address of the first holding registers to be written
 * @param[in] writeQuantity Number of holding registers to be written
 * @param[in] writeValue Value of the holding registers (write operation)
 * @return Error code
 **/

error_t modbusClientReadWriteMultipleRegs(ModbusClientContext *context,
   uint16_t readAddress, uint_t readQuantity, uint16_t *readValue,
   uint16_t writeAddress, uint_t writeQuantity, const uint16_t *writeValue)
{
   error_t error;

   //Check parameters
   if(context == NULL || readValue == NULL || writeValue == NULL)
      return ERROR_INVALID_PARAMETER;

   //The number of registers to be read must be in range 1 to 125
   if(readQuantity < 1 || readQuantity > 125)
      return ERROR_INVALID_PARAMETER;

   //The number of registers to be written must be in range 1 to 121
   if(writeQuantity < 1 || writeQuantity > 121)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Perform Modbus request/response transaction
   while(!error)
   {
      //Check current state
      if(context->state == MODBUS_CLIENT_STATE_CONNECTED)
      {
         //Format request
         error = modbusClientFormatReadWriteMultipleRegsReq(context,
            readAddress, readQuantity, writeAddress, writeQuantity, writeValue);
      }
      else if(context->state == MODBUS_CLIENT_STATE_SENDING ||
         context->state == MODBUS_CLIENT_STATE_RECEIVING)
      {
         //Send Modbus request and wait for a matching response
         error = modbusClientTransaction(context);
      }
      else if(context->state == MODBUS_CLIENT_STATE_COMPLETE)
      {
         //Parse response
         error = modbusClientParseReadWriteMultipleRegsResp(context,
            readQuantity, readValue);

         //The Modbus transaction is complete
         context->state = MODBUS_CLIENT_STATE_CONNECTED;
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Retrieve exception code
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[out] exceptionCode Exception code
 * @return Error code
 **/

error_t modbusClientGetExceptionCode(ModbusClientContext *context,
   ModbusExceptionCode *exceptionCode)
{
   //Check parameters
   if(context == NULL || exceptionCode == NULL)
      return ERROR_INVALID_PARAMETER;

   //Retrieve exception code
   *exceptionCode = context->exceptionCode;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Gracefully disconnect from the Modbus/TCP server
 * @param[in] context Pointer to the Modbus/TCP client context
 * @return Error code
 **/

error_t modbusClientDisconnect(ModbusClientContext *context)
{
   error_t error;

   //Make sure the Modbus/TCP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Gracefully disconnect from the Modbus/TCP server
   while(!error)
   {
      //Check current state
      if(context->state == MODBUS_CLIENT_STATE_CONNECTED)
      {
         //Save current time
         context->timestamp = osGetSystemTime();
         //Update Modbus/TCP client state
         context->state = MODBUS_CLIENT_STATE_DISCONNECTING;
      }
      else if(context->state == MODBUS_CLIENT_STATE_DISCONNECTING)
      {
         //Shutdown connection
         error = modbusClientShutdownConnection(context);

         //Check status code
         if(error == NO_ERROR)
         {
            //Close connection
            modbusClientCloseConnection(context);
            //Update Modbus/TCP client state
            context->state = MODBUS_CLIENT_STATE_DISCONNECTED;
         }
         else if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
         {
            //Check whether the timeout has elapsed
            error = modbusClientCheckTimeout(context);
         }
         else
         {
            //A communication error has occurred
         }
      }
      else if(context->state == MODBUS_CLIENT_STATE_DISCONNECTED)
      {
         //The Modbus/TCP client is disconnected
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }
   }

   //Failed to gracefully disconnect from the Modbus/TCP server?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Close connection
      modbusClientCloseConnection(context);
      //Update Modbus/TCP client state
      context->state = MODBUS_CLIENT_STATE_DISCONNECTED;
   }

   //Return status code
   return error;
}


/**
 * @brief Close the connection with the Modbus/TCP server
 * @param[in] context Pointer to the Modbus/TCP client context
 * @return Error code
 **/

error_t modbusClientClose(ModbusClientContext *context)
{
   //Make sure the Modbus/TCP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Close connection
   modbusClientCloseConnection(context);
   //Update Modbus/TCP client state
   context->state = MODBUS_CLIENT_STATE_DISCONNECTED;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Release Modbus/TCP client context
 * @param[in] context Pointer to the Modbus/TCP client context
 **/

void modbusClientDeinit(ModbusClientContext *context)
{
   //Make sure the Modbus/TCP client context is valid
   if(context != NULL)
   {
      //Close connection
      modbusClientCloseConnection(context);

#if (MODBUS_CLIENT_TLS_SUPPORT == ENABLED)
      //Release TLS session state
      tlsFreeSessionState(&context->tlsSession);
#endif

      //Clear Modbus/TCP client context
      osMemset(context, 0, sizeof(ModbusClientContext));
   }
}

#endif
