/**
 * @file modbus_client_pdu.c
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

//Switch to the appropriate trace level
#define TRACE_LEVEL MODBUS_TRACE_LEVEL

//Dependencies
#include "modbus/modbus_client.h"
#include "modbus/modbus_client_pdu.h"
#include "modbus/modbus_client_misc.h"
#include "modbus/modbus_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MODBUS_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Format Read Coils request
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the first coil
 * @param[in] quantity Number of coils
 * @return Error code
 **/

error_t modbusClientFormatReadCoilsReq(ModbusClientContext *context,
   uint16_t address, uint_t quantity)
{
   size_t length;
   ModbusReadCoilsReq *request;

   //Point to the Modbus request PDU
   request = modbusClientGetRequestPdu(context);

   //Format Read Coils request
   request->functionCode = MODBUS_FUNCTION_READ_COILS;
   request->startingAddr = htons(address);
   request->quantityOfCoils = htons(quantity);

   //Compute the length of the request PDU
   length = sizeof(ModbusReadCoilsReq);

   //Debug message
   TRACE_DEBUG("\r\nModbus Client: Sending Request PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpRequestPdu(request, length);

   //Format MBAP header
   return modbusClientFormatMbapHeader(context, length);
}


/**
 * @brief Format Read Discrete Inputs request
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the first coil
 * @param[in] quantity Number of inputs
 * @return Error code
 **/

error_t modbusClientFormatReadDiscreteInputsReq(ModbusClientContext *context,
   uint16_t address, uint_t quantity)
{
   size_t length;
   ModbusReadDiscreteInputsReq *request;

   //Point to the Modbus request PDU
   request = modbusClientGetRequestPdu(context);

   //Format Read Discrete Inputs request
   request->functionCode = MODBUS_FUNCTION_READ_DISCRETE_INPUTS;
   request->startingAddr = htons(address);
   request->quantityOfInputs = htons(quantity);

   //Compute the length of the request PDU
   length = sizeof(ModbusReadDiscreteInputsReq);

   //Debug message
   TRACE_DEBUG("\r\nModbus Client: Sending Request PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpRequestPdu(request, length);

   //Format MBAP header
   return modbusClientFormatMbapHeader(context, length);
}


/**
 * @brief Format Read Holding Registers request
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Starting register address
 * @param[in] quantity Number of registers
 * @return Error code
 **/

error_t modbusClientFormatReadHoldingRegsReq(ModbusClientContext *context,
   uint16_t address, uint_t quantity)
{
   size_t length;
   ModbusReadHoldingRegsReq *request;

   //Point to the Modbus request PDU
   request = modbusClientGetRequestPdu(context);

   //Format Read Holding Registers request
   request->functionCode = MODBUS_FUNCTION_READ_HOLDING_REGS;
   request->startingAddr = htons(address);
   request->quantityOfRegs = htons(quantity);

   //Compute the length of the request PDU
   length = sizeof(ModbusReadHoldingRegsReq);

   //Debug message
   TRACE_DEBUG("\r\nModbus Client: Sending Request PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpRequestPdu(request, length);

   //Format MBAP header
   return modbusClientFormatMbapHeader(context, length);
}


/**
 * @brief Format Read Input Registers request
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Starting register address
 * @param[in] quantity Number of registers
 * @return Error code
 **/

error_t modbusClientFormatReadInputRegsReq(ModbusClientContext *context,
   uint16_t address, uint_t quantity)
{
   size_t length;
   ModbusReadInputRegsReq *request;

   //Point to the Modbus request PDU
   request = modbusClientGetRequestPdu(context);

   //Format Read Input Registers request
   request->functionCode = MODBUS_FUNCTION_READ_INPUT_REGS;
   request->startingAddr = htons(address);
   request->quantityOfRegs = htons(quantity);

   //Compute the length of the request PDU
   length = sizeof(ModbusReadInputRegsReq);

   //Debug message
   TRACE_DEBUG("\r\nModbus Client: Sending Request PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpRequestPdu(request, length);

   //Format MBAP header
   return modbusClientFormatMbapHeader(context, length);
}


/**
 * @brief Format Write Single Coil request
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the coil to be forced
 * @param[in] value Value of the discrete output
 * @return Error code
 **/

error_t modbusClientFormatWriteSingleCoilReq(ModbusClientContext *context,
   uint16_t address, bool_t value)
{
   size_t length;
   ModbusWriteSingleCoilReq *request;

   //Point to the Modbus request PDU
   request = modbusClientGetRequestPdu(context);

   //Format Write Single Coil request
   request->functionCode = MODBUS_FUNCTION_WRITE_SINGLE_COIL;
   request->outputAddr = htons(address);

   //A value of 0xFF00 requests the output to be ON. A value of 0x0000
   //requests it to be OFF
   if(value)
      request->outputValue = HTONS(MODBUS_COIL_STATE_ON);
   else
      request->outputValue = HTONS(MODBUS_COIL_STATE_OFF);

   //Compute the length of the request PDU
   length = sizeof(ModbusWriteSingleCoilReq);

   //Debug message
   TRACE_DEBUG("\r\nModbus Client: Sending Request PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpRequestPdu(request, length);

   //Format MBAP header
   return modbusClientFormatMbapHeader(context, length);
}


/**
 * @brief Format Write Single Register request
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the register to be written
 * @param[in] value Register value
 * @return Error code
 **/

error_t modbusClientFormatWriteSingleRegReq(ModbusClientContext *context,
   uint16_t address, uint16_t value)
{
   size_t length;
   ModbusWriteSingleRegReq *request;

   //Point to the Modbus request PDU
   request = modbusClientGetRequestPdu(context);

   //Format Write Single Register request
   request->functionCode = MODBUS_FUNCTION_WRITE_SINGLE_REG;
   request->regAddr = htons(address);
   request->regValue = htons(value);

   //Compute the length of the request PDU
   length = sizeof(ModbusWriteSingleRegReq);

   //Debug message
   TRACE_DEBUG("\r\nModbus Client: Sending Request PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpRequestPdu(request, length);

   //Format MBAP header
   return modbusClientFormatMbapHeader(context, length);
}


/**
 * @brief Format Write Multiple Coils request
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the first coil to be forced
 * @param[in] quantity Number of coils
 * @param[in] value Value of the discrete outputs
 * @return Error code
 **/

error_t modbusClientFormatWriteMultipleCoilsReq(ModbusClientContext *context,
   uint16_t address, uint_t quantity, const uint8_t *value)
{
   size_t length;
   ModbusWriteMultipleCoilsReq *request;

   //Point to the Modbus request PDU
   request = modbusClientGetRequestPdu(context);

   //Format Write Multiple Coils request
   request->functionCode = MODBUS_FUNCTION_WRITE_MULTIPLE_COILS;
   request->startingAddr = htons(address);
   request->quantityOfOutputs = htons(quantity);
   request->byteCount = (quantity + 7) / 8;

   //Copy coil values
   osMemcpy(request->outputValue, value, request->byteCount);

   //Compute the length of the request PDU
   length = sizeof(ModbusWriteMultipleCoilsReq) + request->byteCount;

   //Debug message
   TRACE_DEBUG("\r\nModbus Client: Sending Request PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpRequestPdu(request, length);

   //Format MBAP header
   return modbusClientFormatMbapHeader(context, length);
}


/**
 * @brief Format Write Multiple Registers request
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Starting register address
 * @param[in] quantity Number of registers
 * @param[in] value Value of the holding registers
 * @return Error code
 **/

error_t modbusClientFormatWriteMultipleRegsReq(ModbusClientContext *context,
   uint16_t address, uint_t quantity, const uint16_t *value)
{
   uint_t i;
   size_t length;
   ModbusWriteMultipleRegsReq *request;

   //Point to the Modbus request PDU
   request = modbusClientGetRequestPdu(context);

   //Format Write Multiple Registers request
   request->functionCode = MODBUS_FUNCTION_WRITE_MULTIPLE_REGS;
   request->startingAddr = htons(address);
   request->quantityOfRegs = htons(quantity);
   request->byteCount = quantity * sizeof(uint16_t);

   //Copy register values
   for(i = 0; i < quantity; i++)
   {
      request->regValue[i] = ntohs(value[i]);
   }

   //Compute the length of the request PDU
   length = sizeof(ModbusWriteMultipleRegsReq) + request->byteCount;

   //Debug message
   TRACE_DEBUG("\r\nModbus Client: Sending Request PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpRequestPdu(request, length);

   //Format MBAP header
   return modbusClientFormatMbapHeader(context, length);
}


/**
 * @brief Format Mask Write Register request
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the holding register
 * @param[in] andMask AND bitmask
 * @param[in] orMask OR bitmask
 * @return Error code
 **/

error_t modbusClientFormatMaskWriteRegReq(ModbusClientContext *context,
   uint16_t address, uint16_t andMask, uint16_t orMask)
{
   size_t length;
   ModbusMaskWriteRegReq *request;

   //Point to the Modbus request PDU
   request = modbusClientGetRequestPdu(context);

   //Format Write Single Register request
   request->functionCode = MODBUS_FUNCTION_MASK_WRITE_REG;
   request->referenceAddr = htons(address);
   request->andMask = htons(andMask);
   request->orMask = htons(orMask);

   //Compute the length of the request PDU
   length = sizeof(ModbusMaskWriteRegReq);

   //Debug message
   TRACE_DEBUG("\r\nModbus Client: Sending Request PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpRequestPdu(request, length);

   //Format MBAP header
   return modbusClientFormatMbapHeader(context, length);
}


/**
 * @brief Format Read/Write Multiple Registers request
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] readAddress Address of the first holding registers to be read
 * @param[in] readQuantity Number of holding registers to be read
 * @param[in] writeAddress Address of the first holding registers to be written
 * @param[in] writeQuantity Number of holding registers to be written
 * @param[in] writeValue Value of the holding registers (write operation)
 * @return Error code
 **/

error_t modbusClientFormatReadWriteMultipleRegsReq(ModbusClientContext *context,
   uint16_t readAddress, uint16_t readQuantity, uint16_t writeAddress,
   uint16_t writeQuantity, const uint16_t *writeValue)
{
   uint_t i;
   size_t length;
   ModbusReadWriteMultipleRegsReq *request;

   //Point to the Modbus request PDU
   request = modbusClientGetRequestPdu(context);

   //Format Read/Write Multiple Registers request
   request->functionCode = MODBUS_FUNCTION_READ_WRITE_MULTIPLE_REGS;
   request->readStartingAddr = htons(readAddress);
   request->quantityToRead = htons(readQuantity);
   request->writeStartingAddr = htons(writeAddress);
   request->quantityToWrite = htons(writeQuantity);
   request->writeByteCount = writeQuantity * sizeof(uint16_t);

   //Copy register values
   for(i = 0; i < writeQuantity; i++)
   {
      request->writeRegValue[i] = ntohs(writeValue[i]);
   }

   //Compute the length of the request PDU
   length = sizeof(ModbusReadWriteMultipleRegsReq) + request->writeByteCount;

   //Debug message
   TRACE_DEBUG("\r\nModbus Client: Sending Request PDU (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump the contents of the PDU for debugging purpose
   modbusDumpRequestPdu(request, length);

   //Format MBAP header
   return modbusClientFormatMbapHeader(context, length);
}


/**
 * @brief Parse Read Coils response
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] quantity Number of coils
 * @param[out] value Value of the discrete outputs
 * @return Error code
 **/

error_t modbusClientParseReadCoilsResp(ModbusClientContext *context,
   uint_t quantity, uint8_t *value)
{
   size_t n;
   size_t length;
   ModbusReadCoilsResp *response;

   //Point to the Modbus response PDU
   response = modbusClientGetResponsePdu(context, &length);

   //Malformed PDU?
   if(length < sizeof(ModbusReadCoilsResp))
      return ERROR_INVALID_LENGTH;

   //Compute the length of the data field
   n = length - sizeof(ModbusReadCoilsResp);

   //Check function code
   if(response->functionCode != MODBUS_FUNCTION_READ_COILS)
      return ERROR_INVALID_RESPONSE;

   //Check byte count field
   if(response->byteCount != n || response->byteCount != ((quantity + 7) / 8))
      return ERROR_INVALID_LENGTH;

   //Copy coil values
   osMemcpy(value, response->coilStatus, response->byteCount);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Discrete Inputs response
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] quantity Number of inputs
 * @param[out] value Value of the discrete inputs
 * @return Error code
 **/

error_t modbusClientParseReadDiscreteInputsResp(ModbusClientContext *context,
   uint_t quantity, uint8_t *value)
{
   size_t n;
   size_t length;
   ModbusReadDiscreteInputsResp *response;

   //Point to the Modbus response PDU
   response = modbusClientGetResponsePdu(context, &length);

   //Malformed PDU?
   if(length < sizeof(ModbusReadDiscreteInputsResp))
      return ERROR_INVALID_LENGTH;

   //Compute the length of the data field
   n = length - sizeof(ModbusReadDiscreteInputsResp);

   //Check function code
   if(response->functionCode != MODBUS_FUNCTION_READ_DISCRETE_INPUTS)
      return ERROR_INVALID_RESPONSE;

   //Check byte count field
   if(response->byteCount != n || response->byteCount != ((quantity + 7) / 8))
      return ERROR_INVALID_LENGTH;

   //Copy discrete input values
   osMemcpy(value, response->inputStatus, response->byteCount);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Read Holding Registers response
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] quantity Number of registers
 * @param[out] value Value of the holding registers
 * @return Error code
 **/

error_t modbusClientParseReadHoldingRegsResp(ModbusClientContext *context,
   uint_t quantity, uint16_t *value)
{
   uint_t i;
   size_t n;
   size_t length;
   ModbusReadHoldingRegsResp *response;

   //Point to the Modbus response PDU
   response = modbusClientGetResponsePdu(context, &length);

   //Malformed PDU?
   if(length < sizeof(ModbusReadHoldingRegsResp))
      return ERROR_INVALID_LENGTH;

   //Compute the length of the data field
   n = length - sizeof(ModbusReadHoldingRegsResp);

   //Check function code
   if(response->functionCode != MODBUS_FUNCTION_READ_HOLDING_REGS)
      return ERROR_INVALID_RESPONSE;

   //Check byte count field
   if(response->byteCount != n ||
      response->byteCount != (quantity * sizeof(uint16_t)))
   {
      return ERROR_INVALID_LENGTH;
   }

   //Copy register values
   for(i = 0; i < quantity; i++)
   {
      value[i] = ntohs(response->regValue[i]);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Read Input Registers response
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] quantity Number of registers
 * @param[out] value Value of the input registers
 * @return Error code
 **/

error_t modbusClientParseReadInputRegsResp(ModbusClientContext *context,
   uint_t quantity, uint16_t *value)
{
   uint_t i;
   size_t n;
   size_t length;
   ModbusReadInputRegsResp *response;

   //Point to the Modbus response PDU
   response = modbusClientGetResponsePdu(context, &length);

   //Malformed PDU?
   if(length < sizeof(ModbusReadInputRegsResp))
      return ERROR_INVALID_LENGTH;

   //Compute the length of the data field
   n = length - sizeof(ModbusReadInputRegsResp);

   //Check function code
   if(response->functionCode != MODBUS_FUNCTION_READ_INPUT_REGS)
      return ERROR_INVALID_RESPONSE;

   //Check byte count field
   if(response->byteCount != n ||
      response->byteCount != (quantity * sizeof(uint16_t)))
   {
      return ERROR_INVALID_LENGTH;
   }

   //Copy register values
   for(i = 0; i < quantity; i++)
   {
      value[i] = ntohs(response->regValue[i]);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Write Single Coil response
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the coil to be forced
 * @param[in] value Value of the discrete output
 * @return Error code
 **/

error_t modbusClientParseWriteSingleCoilResp(ModbusClientContext *context,
   uint16_t address, bool_t value)
{
   size_t length;
   bool_t referenceValue;
   ModbusWriteSingleCoilResp *response;

   //Point to the Modbus response PDU
   response = modbusClientGetResponsePdu(context, &length);

   //Malformed PDU?
   if(length < sizeof(ModbusWriteSingleCoilResp))
      return ERROR_INVALID_LENGTH;

   //Check function code
   if(response->functionCode != MODBUS_FUNCTION_WRITE_SINGLE_COIL)
      return ERROR_INVALID_RESPONSE;

   //A value of 0xFF00 requests the output to be ON. A value of 0x0000
   //requests it to be OFF
   referenceValue = value ? MODBUS_COIL_STATE_ON : MODBUS_COIL_STATE_OFF;

   //The normal response is an echo of the request
   if(ntohs(response->outputAddr) != address ||
      ntohs(response->outputValue) != referenceValue)
   {
      return ERROR_INVALID_RESPONSE;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Write Single Register response
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the register to be written
 * @param[in] value Register value
 * @return Error code
 **/

error_t modbusClientParseWriteSingleRegResp(ModbusClientContext *context,
   uint16_t address, uint16_t value)
{
   size_t length;
   ModbusWriteSingleRegResp *response;

   //Point to the Modbus response PDU
   response = modbusClientGetResponsePdu(context, &length);

   //Malformed PDU?
   if(length < sizeof(ModbusWriteSingleRegResp))
      return ERROR_INVALID_LENGTH;

   //Check function code
   if(response->functionCode != MODBUS_FUNCTION_WRITE_SINGLE_REG)
      return ERROR_INVALID_RESPONSE;

   //The normal response is an echo of the request
   if(ntohs(response->regAddr) != address ||
      ntohs(response->regValue) != value)
   {
      return ERROR_INVALID_RESPONSE;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Write Multiple Coils response
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the first coil to be forced
 * @param[in] quantity Number of coils
 * @return Error code
 **/

error_t modbusClientParseWriteMultipleCoilsResp(ModbusClientContext *context,
   uint16_t address, uint_t quantity)
{
   size_t length;
   ModbusWriteMultipleCoilsResp *response;

   //Point to the Modbus response PDU
   response = modbusClientGetResponsePdu(context, &length);

   //Malformed PDU?
   if(length < sizeof(ModbusWriteMultipleCoilsResp))
      return ERROR_INVALID_LENGTH;

   //Check function code
   if(response->functionCode != MODBUS_FUNCTION_WRITE_MULTIPLE_COILS)
      return ERROR_INVALID_RESPONSE;

   //The normal response returns the starting address, and quantity of
   //coils forced
   if(ntohs(response->startingAddr) != address ||
      ntohs(response->quantityOfOutputs) != quantity)
   {
      return ERROR_INVALID_RESPONSE;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Write Multiple Registers response
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Starting register address
 * @param[in] quantity Number of registers
 * @return Error code
 **/

error_t modbusClientParseWriteMultipleRegsResp(ModbusClientContext *context,
   uint16_t address, uint_t quantity)
{
   size_t length;
   ModbusWriteMultipleRegsResp *response;

   //Point to the Modbus response PDU
   response = modbusClientGetResponsePdu(context, &length);

   //Malformed PDU?
   if(length < sizeof(ModbusWriteMultipleRegsResp))
      return ERROR_INVALID_LENGTH;

   //Check function code
   if(response->functionCode != MODBUS_FUNCTION_WRITE_MULTIPLE_REGS)
      return ERROR_INVALID_RESPONSE;

   //The normal response returns the starting address, and quantity of
   //registers written
   if(ntohs(response->startingAddr) != address ||
      ntohs(response->quantityOfRegs) != quantity)
   {
      return ERROR_INVALID_RESPONSE;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Mask Write Register response
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] address Address of the holding register
 * @param[in] andMask AND bitmask
 * @param[in] orMask OR bitmask
 * @return Error code
 **/

error_t modbusClientParseMaskWriteRegResp(ModbusClientContext *context,
   uint16_t address, uint16_t andMask, uint16_t orMask)
{
   size_t length;
   ModbusMaskWriteRegResp *response;

   //Point to the Modbus response PDU
   response = modbusClientGetResponsePdu(context, &length);

   //Malformed PDU?
   if(length < sizeof(ModbusMaskWriteRegResp))
      return ERROR_INVALID_LENGTH;

   //Check function code
   if(response->functionCode != MODBUS_FUNCTION_MASK_WRITE_REG)
      return ERROR_INVALID_RESPONSE;

   //The normal response is an echo of the request
   if(ntohs(response->referenceAddr) != address ||
      ntohs(response->andMask) != andMask ||
      ntohs(response->orMask) != orMask)
   {
      return ERROR_INVALID_RESPONSE;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Read/Write Multiple Registers response
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] readQuantity Number of holding registers to be read
 * @param[out] readValue Value of the holding registers
 * @return Error code
 **/

error_t modbusClientParseReadWriteMultipleRegsResp(ModbusClientContext *context,
   uint_t readQuantity, uint16_t *readValue)
{
   uint_t i;
   size_t n;
   size_t length;
   ModbusReadWriteMultipleRegsResp *response;

   //Point to the Modbus response PDU
   response = modbusClientGetResponsePdu(context, &length);

   //Malformed PDU?
   if(length < sizeof(ModbusReadWriteMultipleRegsResp))
      return ERROR_INVALID_LENGTH;

   //Compute the length of the data field
   n = length - sizeof(ModbusReadInputRegsResp);

   //Check function code
   if(response->functionCode != MODBUS_FUNCTION_READ_WRITE_MULTIPLE_REGS)
      return ERROR_INVALID_RESPONSE;

   //Check byte count field
   if(response->readByteCount != n ||
      response->readByteCount != (readQuantity * sizeof(uint16_t)))
   {
      return ERROR_INVALID_LENGTH;
   }

   //Copy register values
   for(i = 0; i < readQuantity; i++)
   {
      readValue[i] = ntohs(response->readRegValue[i]);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Exception response
 * @param[in] context Pointer to the Modbus/TCP client context
 * @return Error code
 **/

error_t modbusClientParseExceptionResp(ModbusClientContext *context)
{
   size_t length;
   ModbusExceptionResp *response;

   //Point to the Modbus response PDU
   response = modbusClientGetResponsePdu(context, &length);

   //Malformed PDU?
   if(length < sizeof(ModbusExceptionResp))
      return ERROR_INVALID_LENGTH;

   //Save exception code
   context->exceptionCode = (ModbusExceptionCode) response->exceptionCode;

   //Send a negative confirmation to the user application
   return ERROR_EXCEPTION_RECEIVED;
}

#endif
