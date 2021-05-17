/**
 * @file modbus_server_pdu.c
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

//Switch to the appropriate trace level
#define TRACE_LEVEL MODBUS_TRACE_LEVEL

//Dependencies
#include "modbus/modbus_server.h"
#include "modbus/modbus_server_pdu.h"
#include "modbus/modbus_server_misc.h"
#include "modbus/modbus_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MODBUS_SERVER_SUPPORT == ENABLED)


/**
 * @brief Process Modbus request
 * @param[in] connection Pointer to the client connection
 * @return Error code
 **/

error_t modbusServerProcessRequest(ModbusClientConnection *connection)
{
   error_t error;
   size_t requestLen;
   size_t responseLen;
   void *request;
   void *response;
   ModbusFunctionCode functionCode;
   ModbusExceptionCode exceptionCode;
   ModbusServerContext *context;

   //Point to the Modbus server context
   context = connection->context;
   //Point to the Modbus request PDU
   request = modbusServerGetRequestPdu(connection, &requestLen);

   //Malformed request?
   if(requestLen == 0)
      return ERROR_INVALID_LENGTH;

   //Debug message
   TRACE_INFO("Modbus Server: Request PDU received (%" PRIuSIZE " bytes)...\r\n",
      requestLen);

   //Dump the contents of the PDU for debugging purpose
   modbusDumpRequestPdu(request, requestLen);

   //Retrieve function code
   functionCode = (ModbusFunctionCode) ((uint8_t *) request)[0];

   //Any registered callback?
   if(context->settings.processPduCallback != NULL)
   {
      //Point to the Modbus response PDU
      response = modbusServerGetResponsePdu(connection);
      //Initialize response length
      responseLen = 0;

      //Process request PDU
      error = context->settings.processPduCallback(request, requestLen,
         response, &responseLen);

      //Check status code
      if(!error)
      {
         //Valid response PDU?
         if(responseLen > 0)
         {
            //Debug message
            TRACE_INFO("Modbus Server: Sending Response PDU (%" PRIuSIZE " bytes)...\r\n",
               responseLen);

            //Dump the contents of the PDU for debugging purpose
            modbusDumpResponsePdu(response, responseLen);

            //Format MBAP header
            error = modbusServerFormatMbapHeader(connection, responseLen);
         }
      }
   }
   else
   {
      //Keep processing
      error = ERROR_INVALID_FUNCTION_CODE;
   }

   //Unknown function code?
   if(error == ERROR_INVALID_FUNCTION_CODE)
   {
      //Check function code
      switch(functionCode)
      {
      //Read Coils request?
      case MODBUS_FUNCTION_READ_COILS:
         //Process Modbus PDU
         error = modbusServerProcessReadCoilsReq(connection,
            request, requestLen);
         break;

      //Format Read Discrete Inputs request?
      case MODBUS_FUNCTION_READ_DISCRETE_INPUTS:
         //Process Modbus PDU
         error = modbusServerProcessReadDiscreteInputsReq(connection,
            request, requestLen);
         break;

      //Read Holding Registers request?
      case MODBUS_FUNCTION_READ_HOLDING_REGS:
         //Process Modbus PDU
         error = modbusServerProcessReadHoldingRegsReq(connection,
            request, requestLen);
         break;

      //Read Input Registers request?
      case MODBUS_FUNCTION_READ_INPUT_REGS:
         //Process Modbus PDU
         error = modbusServerProcessReadInputRegsReq(connection,
            request, requestLen);
         break;

      //Write Single Coil request?
      case MODBUS_FUNCTION_WRITE_SINGLE_COIL:
         //Process Modbus PDU
         error = modbusServerProcessWriteSingleCoilReq(connection,
            request, requestLen);
         break;

      //Write Single Register request?
      case MODBUS_FUNCTION_WRITE_SINGLE_REG:
         //Process Modbus PDU
         error = modbusServerProcessWriteSingleRegReq(connection,
            request, requestLen);
         break;

      //Write Multiple Coils request?
      case MODBUS_FUNCTION_WRITE_MULTIPLE_COILS:
         //Process Modbus PDU
         error = modbusServerProcessWriteMultipleCoilsReq(connection,
            request, requestLen);
         break;

      //Write Multiple Registers request?
      case MODBUS_FUNCTION_WRITE_MULTIPLE_REGS:
         //Process Modbus PDU
         error = modbusServerProcessWriteMultipleRegsReq(connection,
            request, requestLen);
         break;

      //Mask Write Register request?
      case MODBUS_FUNCTION_MASK_WRITE_REG:
         //Process Modbus PDU
         error = modbusServerProcessMaskWriteRegReq(connection,
            request, requestLen);
         break;

      //Read/Write Multiple Registers request?
      case MODBUS_FUNCTION_READ_WRITE_MULTIPLE_REGS:
         //Process Modbus PDU
         error = modbusServerProcessReadWriteMultipleRegsReq(connection,
            request, requestLen);
         break;

      //Illegal function code?
      default:
         //Report an error
         error = ERROR_INVALID_FUNCTION_CODE;
         break;
      }
   }

   //Any exception?
   if(error == ERROR_INVALID_FUNCTION_CODE ||
      error == ERROR_INVALID_ADDRESS ||
      error == ERROR_INVALID_VALUE ||
      error == ERROR_WRITE_FAILED ||
      error == ERROR_READ_FAILED ||
      error == ERROR_DEVICE_BUSY)
   {
      //Retrieve exception code
      exceptionCode = modbusServerTranslateExceptionCode(error);

      //Send an exception response to the Modbus/TCP client
      error = modbusServerFormatExceptionResp(connection, functionCode,
         exceptionCode);
   }

   //Return status code
   return error;
}


/**
 * @brief Process Read Coils request
 * @param[in] connection Pointer to the client connection
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusServerProcessReadCoilsReq(ModbusClientConnection *connection,
   const ModbusReadCoilsReq *request, size_t length)
{
   error_t error;
   uint16_t i;
   uint16_t quantity;
   uint16_t address;
   bool_t state;
   ModbusReadCoilsResp *response;

   //Initialize status code
   error = NO_ERROR;

   //Malformed PDU?
   if(length < sizeof(ModbusReadCoilsReq))
      return ERROR_INVALID_LENGTH;

   //Get the address of the first coil
   address = ntohs(request->startingAddr);
   //Get the number of coils
   quantity = ntohs(request->quantityOfCoils);

   //The number of discrete inputs must be in range 1 to 2000
   if(quantity < 1 || quantity > 2000)
      return ERROR_INVALID_VALUE;

   //Point to the Modbus response PDU
   response = modbusServerGetResponsePdu(connection);

   //Format Read Coils response
   response->functionCode = request->functionCode;
   response->byteCount = (quantity + 7) / 8;

   //If the quantity of coils is not a multiple of eight, the remaining
   //bits in the final data byte will be padded with zeros
   if((quantity % 8) != 0)
      response->coilStatus[response->byteCount - 1] = 0;

   //Lock access to Modbus table
   modbusServerLock(connection);

   //Read the specified number of coils
   for(i = 0; i < quantity && !error; i++)
   {
      //Retrieve the state of the current coil
      error = modbusServerReadCoil(connection, address + i, &state);

      //Successful read operation?
      if(!error)
      {
         //The coils in the response message are packed as one coil per bit
         //of the data field
         if(state)
            MODBUS_SET_COIL(response->coilStatus, i);
         else
            MODBUS_RESET_COIL(response->coilStatus, i);
      }
   }

   //Unlock access to Modbus table
   modbusServerUnlock(connection);

   //Check whether the read operation has failed
   if(error)
      return error;

   //Compute the length of the response PDU
   length = sizeof(ModbusReadCoilsResp) + response->byteCount;

   //Debug message
   TRACE_INFO("Modbus Server: Sending Response PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpResponsePdu(response, length);

   //Format MBAP header
   return modbusServerFormatMbapHeader(connection, length);
}


/**
 * @brief Process Read Discrete Inputs request
 * @param[in] connection Pointer to the client connection
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusServerProcessReadDiscreteInputsReq(ModbusClientConnection *connection,
   const ModbusReadDiscreteInputsReq *request, size_t length)
{
   error_t error;
   uint16_t i;
   uint16_t address;
   uint16_t quantity;
   bool_t state;
   ModbusReadDiscreteInputsResp *response;

   //Initialize status code
   error = NO_ERROR;

   //Malformed PDU?
   if(length < sizeof(ModbusReadDiscreteInputsReq))
      return ERROR_INVALID_LENGTH;

   //Get the address of the first coil
   address = ntohs(request->startingAddr);
   //Get the number of coils
   quantity = ntohs(request->quantityOfInputs);

   //The number of discrete inputs must be in range 1 to 2000
   if(quantity < 1 || quantity > 2000)
      return ERROR_INVALID_VALUE;

   //Point to the Modbus response PDU
   response = modbusServerGetResponsePdu(connection);

   //Format Read Discrete Inputs response
   response->functionCode = request->functionCode;
   response->byteCount = (quantity + 7) / 8;

   //If the quantity of coils is not a multiple of eight, the remaining
   //bits in the final data byte will be padded with zeros
   if((quantity % 8) != 0)
      response->inputStatus[response->byteCount - 1] = 0;

   //Lock access to Modbus table
   modbusServerLock(connection);

   //Read the specified number of coils
   for(i = 0; i < quantity && !error; i++)
   {
      //Retrieve the state of the current coil
      error = modbusServerReadDiscreteInput(connection, address + i, &state);

      //Successful read operation?
      if(!error)
      {
         //The coils in the response message are packed as one coil per bit
         //of the data field
         if(state)
            MODBUS_SET_COIL(response->inputStatus, i);
         else
            MODBUS_RESET_COIL(response->inputStatus, i);
      }
   }

   //Unlock access to Modbus table
   modbusServerUnlock(connection);

   //Check whether the read operation has failed
   if(error)
      return error;

   //Compute the length of the response PDU
   length = sizeof(ModbusReadDiscreteInputsResp) + response->byteCount;

   //Debug message
   TRACE_INFO("Modbus Server: Sending Response PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpResponsePdu(response, length);

   //Format MBAP header
   return modbusServerFormatMbapHeader(connection, length);
}


/**
 * @brief Process Read Holding Registers request
 * @param[in] connection Pointer to the client connection
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusServerProcessReadHoldingRegsReq(ModbusClientConnection *connection,
   const ModbusReadHoldingRegsReq *request, size_t length)
{
   error_t error;
   uint16_t i;
   uint16_t address;
   uint16_t quantity;
   uint16_t value;
   ModbusReadHoldingRegsResp *response;

   //Initialize status code
   error = NO_ERROR;

   //Malformed PDU?
   if(length < sizeof(ModbusReadHoldingRegsReq))
      return ERROR_INVALID_LENGTH;

   //Get the address of the first register
   address = ntohs(request->startingAddr);
   //Get the number of registers
   quantity = ntohs(request->quantityOfRegs);

   //The number of registers must be in range 1 to 125
   if(quantity < 1 || quantity > 125)
      return ERROR_INVALID_VALUE;

   //Point to the Modbus response PDU
   response = modbusServerGetResponsePdu(connection);

   //Format Read Holding Registers response
   response->functionCode = request->functionCode;
   response->byteCount = quantity * sizeof(uint16_t);

   //Lock access to Modbus table
   modbusServerLock(connection);

   //Read the specified number of registers
   for(i = 0; i < quantity && !error; i++)
   {
      //Retrieve the value of the current register
      error = modbusServerReadHoldingReg(connection, address + i, &value);
      //Convert the value to network byte order
      response->regValue[i] = htons(value);
   }

   //Unlock access to Modbus table
   modbusServerUnlock(connection);

   //Check whether the read operation has failed
   if(error)
      return error;

   //Compute the length of the response PDU
   length = sizeof(ModbusReadHoldingRegsResp) + response->byteCount;

   //Debug message
   TRACE_INFO("Modbus Server: Sending Response PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpResponsePdu(response, length);

   //Format MBAP header
   return modbusServerFormatMbapHeader(connection, length);
}


/**
 * @brief Process Read Input Registers request
 * @param[in] connection Pointer to the client connection
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusServerProcessReadInputRegsReq(ModbusClientConnection *connection,
   const ModbusReadInputRegsReq *request, size_t length)
{
   error_t error;
   uint16_t i;
   uint16_t address;
   uint16_t quantity;
   uint16_t value;
   ModbusReadInputRegsResp *response;

   //Initialize status code
   error = NO_ERROR;

   //Malformed PDU?
   if(length < sizeof(ModbusReadInputRegsReq))
      return ERROR_INVALID_LENGTH;

   //Get the address of the first register
   address = ntohs(request->startingAddr);
   //Get the number of registers
   quantity = ntohs(request->quantityOfRegs);

   //The number of registers must be in range 1 to 125
   if(quantity < 1 || quantity > 125)
      return ERROR_INVALID_VALUE;

   //Point to the Modbus response PDU
   response = modbusServerGetResponsePdu(connection);

   //Format Read Input Registers response
   response->functionCode = request->functionCode;
   response->byteCount = quantity * sizeof(uint16_t);

   //Lock access to Modbus table
   modbusServerLock(connection);

   //Read the specified number of registers
   for(i = 0; i < quantity && !error; i++)
   {
      //Retrieve the value of the current register
      error = modbusServerReadInputReg(connection, address + i, &value);
      //Convert the value to network byte order
      response->regValue[i] = htons(value);
   }

   //Unlock access to Modbus table
   modbusServerUnlock(connection);

   //Check whether the read operation has failed
   if(error)
      return error;

   //Compute the length of the response PDU
   length = sizeof(ModbusReadInputRegsResp) + response->byteCount;

   //Debug message
   TRACE_INFO("Modbus Server: Sending Response PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpResponsePdu(response, length);

   //Format MBAP header
   return modbusServerFormatMbapHeader(connection, length);
}


/**
 * @brief Process Write Single Coil request
 * @param[in] connection Pointer to the client connection
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusServerProcessWriteSingleCoilReq(ModbusClientConnection *connection,
   const ModbusWriteSingleCoilReq *request, size_t length)
{
   error_t error;
   uint16_t address;
   bool_t state;
   ModbusWriteSingleCoilResp *response;

   //Malformed PDU?
   if(length < sizeof(ModbusWriteSingleCoilReq))
      return ERROR_INVALID_LENGTH;

   //Get the address of the coil to be forced
   address = ntohs(request->outputAddr);

   //Check output value
   if(ntohs(request->outputValue) == MODBUS_COIL_STATE_ON)
   {
      //A value of 0xFF00 requests the output to be ON
      state = TRUE;
   }
   else if(ntohs(request->outputValue) == MODBUS_COIL_STATE_OFF)
   {
      //A value of 0x0000 requests the output to be OFF
      state = FALSE;
   }
   else
   {
      //Report an error
      return ERROR_INVALID_VALUE;
   }

   //Lock access to Modbus table
   modbusServerLock(connection);
   //Force the coil to the desired ON/OFF state
   error = modbusServerWriteCoil(connection, address, state, TRUE);
   //Unlock access to Modbus table
   modbusServerUnlock(connection);

   //Check whether the write operation has failed
   if(error)
      return error;

   //Point to the Modbus response PDU
   response = modbusServerGetResponsePdu(connection);

   //The normal response is an echo of the request
   response->functionCode = request->functionCode;
   response->outputAddr = request->outputAddr;
   response->outputValue = request->outputValue;

   //Compute the length of the response PDU
   length = sizeof(ModbusWriteSingleCoilResp);

   //Debug message
   TRACE_INFO("Modbus Server: Sending Response PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpResponsePdu(response, length);

   //Format MBAP header
   return modbusServerFormatMbapHeader(connection, length);
}


/**
 * @brief Process Write Single Register request
 * @param[in] connection Pointer to the client connection
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusServerProcessWriteSingleRegReq(ModbusClientConnection *connection,
   const ModbusWriteSingleRegReq *request, size_t length)
{
   error_t error;
   uint16_t address;
   uint16_t value;
   ModbusWriteSingleRegResp *response;

   //Malformed PDU?
   if(length < sizeof(ModbusWriteSingleRegReq))
      return ERROR_INVALID_LENGTH;

   //Get the address of the register
   address = ntohs(request->regAddr);
   //Get the value of the register
   value = ntohs(request->regValue);

   //Lock access to Modbus table
   modbusServerLock(connection);
   //Write register value
   error = modbusServerWriteReg(connection, address, value, TRUE);
   //Unlock access to Modbus table
   modbusServerUnlock(connection);

   //Check whether the write operation has failed
   if(error)
      return error;

   //Point to the Modbus response PDU
   response = modbusServerGetResponsePdu(connection);

   //The normal response is an echo of the request
   response->functionCode = request->functionCode;
   response->regAddr = request->regAddr;
   response->regValue = request->regValue;

   //Compute the length of the response PDU
   length = sizeof(ModbusWriteSingleRegResp);

   //Debug message
   TRACE_INFO("Modbus Server: Sending Response PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpResponsePdu(response, length);

   //Format MBAP header
   return modbusServerFormatMbapHeader(connection, length);
}


/**
 * @brief Process Write Multiple Coils request
 * @param[in] connection Pointer to the client connection
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusServerProcessWriteMultipleCoilsReq(ModbusClientConnection *connection,
   const ModbusWriteMultipleCoilsReq *request, size_t length)
{
   error_t error;
   uint16_t i;
   uint16_t address;
   uint16_t quantity;
   ModbusWriteMultipleCoilsResp *response;

   //Initialize status code
   error = NO_ERROR;

   //Malformed PDU?
   if(length < sizeof(ModbusWriteMultipleCoilsReq))
      return ERROR_INVALID_LENGTH;

   //Compute the length of the data field
   length -= sizeof(ModbusWriteMultipleCoilsReq);

   //Malformed PDU?
   if(length < request->byteCount)
      return ERROR_INVALID_LENGTH;

   //Get the address of the first coil to be forced
   address = ntohs(request->startingAddr);
   //Get the number of coils
   quantity = ntohs(request->quantityOfOutputs);

   //The number of discrete inputs must be in range 1 to 2000
   if(quantity < 1 || quantity > 2000)
      return ERROR_INVALID_VALUE;

   //Check byte count field
   if(request->byteCount != ((quantity + 7) / 8))
      return ERROR_INVALID_VALUE;

   //Lock access to Modbus table
   modbusServerLock(connection);

   //Consistency check (first phase)
   for(i = 0; i < quantity && !error; i++)
   {
      //Validate coil address
      error = modbusServerWriteCoil(connection, address + i,
         MODBUS_TEST_COIL(request->outputValue, i), FALSE);
   }

   //Commit changes (second phase)
   for(i = 0; i < quantity && !error; i++)
   {
      //Force the current coil to the desired ON/OFF state
      error = modbusServerWriteCoil(connection, address + i,
         MODBUS_TEST_COIL(request->outputValue, i), TRUE);
   }

   //Unlock access to Modbus table
   modbusServerUnlock(connection);

   //Check whether the write operation has failed
   if(error)
      return error;

   //Point to the Modbus response PDU
   response = modbusServerGetResponsePdu(connection);

   //The normal response returns the starting address, and quantity of
   //coils forced
   response->functionCode = request->functionCode;
   response->startingAddr = request->startingAddr;
   response->quantityOfOutputs = request->quantityOfOutputs;

   //Compute the length of the response PDU
   length = sizeof(ModbusWriteMultipleCoilsResp);

   //Debug message
   TRACE_INFO("Modbus Server: Sending Response PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpResponsePdu(response, length);

   //Format MBAP header
   return modbusServerFormatMbapHeader(connection, length);
}


/**
 * @brief Process Write Multiple Registers request
 * @param[in] connection Pointer to the client connection
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusServerProcessWriteMultipleRegsReq(ModbusClientConnection *connection,
   const ModbusWriteMultipleRegsReq *request, size_t length)
{
   error_t error;
   uint16_t i;
   uint16_t address;
   uint16_t quantity;
   ModbusWriteMultipleRegsResp *response;

   //Initialize status code
   error = NO_ERROR;

   //Malformed PDU?
   if(length < sizeof(ModbusWriteMultipleRegsReq))
      return ERROR_INVALID_LENGTH;

   //Compute the length of the data field
   length -= sizeof(ModbusWriteMultipleRegsReq);

   //Malformed PDU?
   if(length < request->byteCount)
      return ERROR_INVALID_LENGTH;

   //Get the address of the first register to be written
   address = ntohs(request->startingAddr);
   //Get the number of registers
   quantity = ntohs(request->quantityOfRegs);

   //The number of registers must be in range 1 to 123
   if(quantity < 1 || quantity > 123)
      return ERROR_INVALID_VALUE;

   //Check byte count field
   if(request->byteCount != (quantity * sizeof(uint16_t)))
      return ERROR_INVALID_VALUE;

   //Lock access to Modbus table
   modbusServerLock(connection);

   //Consistency check (first phase)
   for(i = 0; i < quantity && !error; i++)
   {
      //Validate register address
      error = modbusServerWriteReg(connection, address + i,
         ntohs(request->regValue[i]), FALSE);
   }

   //Commit changes (second phase)
   for(i = 0; i < quantity && !error; i++)
   {
      //Write the value of the current register
      error = modbusServerWriteReg(connection, address + i,
         ntohs(request->regValue[i]), TRUE);
   }

   //Unlock access to Modbus table
   modbusServerUnlock(connection);

   //Check whether the write operation has failed
   if(error)
      return error;

   //Point to the Modbus response PDU
   response = modbusServerGetResponsePdu(connection);

   //The normal response returns the starting address, and quantity of
   //registers written
   response->functionCode = request->functionCode;
   response->startingAddr = request->startingAddr;
   response->quantityOfRegs = request->quantityOfRegs;

   //Compute the length of the response PDU
   length = sizeof(ModbusWriteMultipleRegsResp);

   //Debug message
   TRACE_INFO("Modbus Server: Sending Response PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpResponsePdu(response, length);

   //Format MBAP header
   return modbusServerFormatMbapHeader(connection, length);
}


/**
 * @brief Process Mask Write Register request
 * @param[in] connection Pointer to the client connection
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusServerProcessMaskWriteRegReq(ModbusClientConnection *connection,
   const ModbusMaskWriteRegReq *request, size_t length)
{
   error_t error;
   uint16_t address;
   uint16_t andMask;
   uint16_t orMask;
   uint16_t value;
   ModbusMaskWriteRegResp *response;

   //Malformed PDU?
   if(length < sizeof(ModbusMaskWriteRegReq))
      return ERROR_INVALID_LENGTH;

   //Get the address of the register
   address = ntohs(request->referenceAddr);
   //Get the value of the AND mask
   andMask = ntohs(request->andMask);
   //Get the value of the OR mask
   orMask = ntohs(request->orMask);

   //Lock access to Modbus table
   modbusServerLock(connection);

   //Retrieve the value of the register
   error = modbusServerReadHoldingReg(connection, address, &value);

   //Check status code
   if(!error)
   {
      //Apply AND mask and OR mask
      value = (value & andMask) | (orMask & ~andMask);
      //Write register value
      error = modbusServerWriteReg(connection, address, value, TRUE);
   }

   //Unlock access to Modbus table
   modbusServerUnlock(connection);

   //Check whether the write operation has failed
   if(error)
      return error;

   //Point to the Modbus response PDU
   response = modbusServerGetResponsePdu(connection);

   //The normal response is an echo of the request
   response->functionCode = request->functionCode;
   response->referenceAddr = request->referenceAddr;
   response->andMask = request->andMask;
   response->orMask = request->orMask;

   //Compute the length of the response PDU
   length = sizeof(ModbusMaskWriteRegResp);

   //Debug message
   TRACE_INFO("Modbus Server: Sending Response PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpResponsePdu(response, length);

   //Format MBAP header
   return modbusServerFormatMbapHeader(connection, length);
}


/**
 * @brief Process Read/Write Multiple Registers request
 * @param[in] connection Pointer to the client connection
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusServerProcessReadWriteMultipleRegsReq(ModbusClientConnection *connection,
   const ModbusReadWriteMultipleRegsReq *request, size_t length)
{
   error_t error;
   uint16_t i;
   uint16_t readAddress;
   uint16_t readQuantity;
   uint16_t writeAddress;
   uint16_t writeQuantity;
   uint16_t value;
   ModbusReadWriteMultipleRegsResp *response;

   //Initialize status code
   error = NO_ERROR;

   //Malformed PDU?
   if(length < sizeof(ModbusReadWriteMultipleRegsReq))
      return ERROR_INVALID_LENGTH;

   //Compute the length of the data field
   length -= sizeof(ModbusWriteMultipleRegsReq);

   //Malformed PDU?
   if(length < request->writeByteCount)
      return ERROR_INVALID_LENGTH;

   //Get the address of the first register to be read
   readAddress = ntohs(request->readStartingAddr);
   //Get the number of registers to be read
   readQuantity = ntohs(request->quantityToRead);
   //Get the address of the first register to be written
   writeAddress = ntohs(request->writeStartingAddr);
   //Get the number of registers to be written
   writeQuantity = ntohs(request->quantityToWrite);

   //The number of registers to be read must be in range 1 to 125
   if(readQuantity < 1 || readQuantity > 125)
      return ERROR_INVALID_VALUE;

   //The number of registers to be written must be in range 1 to 121
   if(writeQuantity < 1 || writeQuantity > 121)
      return ERROR_INVALID_VALUE;

   //Check byte count field
   if(request->writeByteCount != (writeQuantity * sizeof(uint16_t)))
      return ERROR_INVALID_VALUE;

   //Point to the Modbus response PDU
   response = modbusServerGetResponsePdu(connection);

   //Format Read/Write Multiple Registers response
   response->functionCode = request->functionCode;
   response->readByteCount = readQuantity * sizeof(uint16_t);

   //Lock access to Modbus table
   modbusServerLock(connection);

   //Read the specified number of registers
   for(i = 0; i < readQuantity && !error; i++)
   {
      //Retrieve the value of the current register
      error = modbusServerReadHoldingReg(connection, readAddress + i, &value);

      //Convert the value to network byte order
      response->readRegValue[i] = htons(value);
   }

   //Consistency check (first phase)
   for(i = 0; i < writeQuantity && !error; i++)
   {
      //Validate register address
      error = modbusServerWriteReg(connection, writeAddress + i,
         ntohs(request->writeRegValue[i]), FALSE);
   }

   //Commit changes (second phase)
   for(i = 0; i < writeQuantity && !error; i++)
   {
      //Write the value of the current register
      error = modbusServerWriteReg(connection, writeAddress + i,
         ntohs(request->writeRegValue[i]), TRUE);
   }

   //Unlock access to Modbus table
   modbusServerUnlock(connection);

   //Check whether the write operation has failed
   if(error)
      return error;

   //Compute the length of the response PDU
   length = sizeof(ModbusReadWriteMultipleRegsResp) + response->readByteCount;

   //Debug message
   TRACE_INFO("Modbus Server: Sending Response PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpResponsePdu(response, length);

   //Format MBAP header
   return modbusServerFormatMbapHeader(connection, length);
}


/**
 * @brief Format exception response
 * @param[in] connection Pointer to the client connection
 * @param[in] functionCode Function code of the request
 * @param[in] exceptionCode Exception code
 * @return Exception code
 **/

error_t modbusServerFormatExceptionResp(ModbusClientConnection *connection,
   ModbusFunctionCode functionCode, ModbusExceptionCode exceptionCode)
{
   size_t length;
   ModbusExceptionResp *response;

   //Point to the Modbus response PDU
   response = modbusServerGetResponsePdu(connection);

   //Format Exception response
   response->functionCode = MODBUS_EXCEPTION_MASK | functionCode;
   response->exceptionCode = exceptionCode;

   //Compute the length of the response PDU
   length = sizeof(ModbusExceptionResp);

   //Debug message
   TRACE_INFO("Modbus Server: Sending Response PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpResponsePdu(response, length);

   //Format MBAP header
   return modbusServerFormatMbapHeader(connection, length);
}

#endif
