/**
 * @file modbus_debug.c
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

//Switch to the appropriate trace level
#define TRACE_LEVEL MODBUS_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "modbus/modbus_client.h"
#include "modbus/modbus_server.h"
#include "modbus/modbus_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MODBUS_CLIENT_SUPPORT == ENABLED || MODBUS_SERVER_SUPPORT == ENABLED)

//Modbus functions codes
const char_t *modbusFunctionCodeLabel[] =
{
   "",                              //0
   "Read Coils",                    //1
   "Read Discrete Inputs",          //2
   "Read Holding Registers",        //3
   "Read Input Registers",          //4
   "Write Single Coil",             //5
   "Write Single Register",         //6
   "Read Exception Status",         //7
   "Diagnostics",                   //8
   "",                              //9
   "",                              //10
   "Get Comm Event Counter",        //11
   "Get Comm Event Log",            //12
   "",                              //13
   "",                              //14
   "Write Multiple Coils",          //15
   "Write Multiple Registers",      //16
   "Report Slave ID",               //17
   "",                              //18
   "",                              //19
   "Read File Record",              //20
   "Write File Record",             //21
   "Mask Write Register",           //22
   "Read/Write Multiple Registers", //23
   "Read FIFO Queue"                //24
};

//Modbus exception codes
const char_t *modbusExceptionCodeLabel[] =
{
   "",                                       //0
   "Illegal Function",                       //1
   "Illegal Data Address",                   //2
   "Illegal Data Value",                     //3
   "Slave Device Failure",                   //4
   "Acknowledge",                            //5
   "Slave Device Busy",                      //6
   "",                                       //7
   "Memory Parity Error",                    //8
   "",                                       //9
   "Gateway Path Unavailable",               //10
   "Gateway Target Device Failed To Respond" //11
};


/**
 * @brief Dump Modbus request PDU for debugging purpose
 * @param[in] pdu Pointer to the Modbus request PDU
 * @param[in] length Length of the PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpRequestPdu(const void *pdu, size_t length)
{
#if (MODBUS_TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   error_t error;
   uint8_t functionCode;
   const char_t *label;

   //Sanity check
   if(length == 0)
      return ERROR_INVALID_LENGTH;

   //Retrieve function code
   functionCode = *((uint8_t *) pdu);

   //Retrieve the name associated with the function code
   if(functionCode < arraysize(modbusFunctionCodeLabel))
      label = modbusFunctionCodeLabel[functionCode];
   else
      label = "";

   //Dump function code
   TRACE_DEBUG("  Function Code = %" PRIu8 " (%s)\r\n", functionCode, label);

   //Check function code
   switch(functionCode)
   {
   //Read Coils request?
   case MODBUS_FUNCTION_READ_COILS:
      //Dump Modbus PDU
      error = modbusDumpReadCoilsReq(pdu, length);
      break;
   //Format Read Discrete Inputs request?
   case MODBUS_FUNCTION_READ_DISCRETE_INPUTS:
      //Dump Modbus PDU
      error = modbusDumpReadDiscreteInputsReq(pdu, length);
      break;
   //Read Holding Registers request?
   case MODBUS_FUNCTION_READ_HOLDING_REGS:
      //Dump Modbus PDU
      error = modbusDumpReadHoldingRegsReq(pdu, length);
      break;
   //Read Input Registers request?
   case MODBUS_FUNCTION_READ_INPUT_REGS:
      //Dump Modbus PDU
      error = modbusDumpReadInputRegsReq(pdu, length);
      break;
   //Write Single Coil request?
   case MODBUS_FUNCTION_WRITE_SINGLE_COIL:
      //Dump Modbus PDU
      error = modbusDumpWriteSingleCoilReq(pdu, length);
      break;
   //Write Single Register request?
   case MODBUS_FUNCTION_WRITE_SINGLE_REG:
      //Dump Modbus PDU
      error = modbusDumpWriteSingleRegReq(pdu, length);
      break;
   //Write Multiple Coils request?
   case MODBUS_FUNCTION_WRITE_MULTIPLE_COILS:
      //Dump Modbus PDU
      error = modbusDumpWriteMultipleCoilsReq(pdu, length);
      break;
   //Write Multiple Registers request?
   case MODBUS_FUNCTION_WRITE_MULTIPLE_REGS:
      //Dump Modbus PDU
      error = modbusDumpWriteMultipleRegsReq(pdu, length);
      break;
   //Mask Write Register request?
   case MODBUS_FUNCTION_MASK_WRITE_REG:
      //Dump Modbus PDU
      error = modbusDumpMaskWriteRegReq(pdu, length);
      break;
   //Read/Write Multiple Registers request?
   case MODBUS_FUNCTION_READ_WRITE_MULTIPLE_REGS:
      //Dump Modbus PDU
      error = modbusDumpReadWriteMultipleRegsReq(pdu, length);
      break;
   //Illegal function code?
   default:
      //Report an error
      error = ERROR_INVALID_FUNCTION_CODE;
      break;
   }

   //Return error code
   return error;
#else
   //Not implemented
   return NO_ERROR;
#endif
}


/**
 * @brief Dump Modbus response PDU for debugging purpose
 * @param[in] pdu Pointer to the Modbus response PDU
 * @param[in] length Length of the PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpResponsePdu(const void *pdu, size_t length)
{
#if (MODBUS_TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   error_t error;
   uint8_t functionCode;
   const char_t *label;

   //Sanity check
   if(length == 0)
      return ERROR_INVALID_LENGTH;

   //Retrieve function code
   functionCode = *((uint8_t *) pdu);

   //Exception response?
   if((functionCode & MODBUS_EXCEPTION_MASK) != 0)
   {
      //Dump Modbus PDU
      error = modbusDumpExceptionResp(pdu, length);
   }
   else
   {
      //Retrieve the name associated with the function code
      if(functionCode < arraysize(modbusFunctionCodeLabel))
         label = modbusFunctionCodeLabel[functionCode];
      else
         label = "";

      //Dump function code
      TRACE_DEBUG("  Function Code = %" PRIu8 " (%s)\r\n", functionCode, label);

      //Check function code
      switch(functionCode)
      {
      //Read Coils response?
      case MODBUS_FUNCTION_READ_COILS:
         //Dump Modbus PDU
         error = modbusDumpReadCoilsResp(pdu, length);
         break;
      //Format Read Discrete Inputs response?
      case MODBUS_FUNCTION_READ_DISCRETE_INPUTS:
         //Dump Modbus PDU
         error = modbusDumpReadDiscreteInputsResp(pdu, length);
         break;
      //Read Holding Registers response?
      case MODBUS_FUNCTION_READ_HOLDING_REGS:
         //Dump Modbus PDU
         error = modbusDumpReadHoldingRegsResp(pdu, length);
         break;
      //Read Input Registers response?
      case MODBUS_FUNCTION_READ_INPUT_REGS:
         //Dump Modbus PDU
         error = modbusDumpReadInputRegsResp(pdu, length);
         break;
      //Write Single Coil response?
      case MODBUS_FUNCTION_WRITE_SINGLE_COIL:
         //Dump Modbus PDU
         error = modbusDumpWriteSingleCoilResp(pdu, length);
         break;
      //Write Single Register response?
      case MODBUS_FUNCTION_WRITE_SINGLE_REG:
         //Dump Modbus PDU
         error = modbusDumpWriteSingleRegResp(pdu, length);
         break;
      //Write Multiple Coils response?
      case MODBUS_FUNCTION_WRITE_MULTIPLE_COILS:
         //Dump Modbus PDU
         error = modbusDumpWriteMultipleCoilsResp(pdu, length);
         break;
      //Write Multiple Registers response?
      case MODBUS_FUNCTION_WRITE_MULTIPLE_REGS:
         //Dump Modbus PDU
         error = modbusDumpWriteMultipleRegsResp(pdu, length);
         break;
      //Mask Write Register response?
      case MODBUS_FUNCTION_MASK_WRITE_REG:
         //Dump Modbus PDU
         error = modbusDumpMaskWriteRegResp(pdu, length);
         break;
      //Read/Write Multiple Registers response?
      case MODBUS_FUNCTION_READ_WRITE_MULTIPLE_REGS:
         //Dump Modbus PDU
         error = modbusDumpReadWriteMultipleRegsResp(pdu, length);
         break;
      //Illegal function code?
      default:
         //Report an error
         error = ERROR_INVALID_FUNCTION_CODE;
         break;
      }
   }

   //Return error code
   return error;
#else
   //Not implemented
   return NO_ERROR;
#endif
}


/**
 * @brief Dump Read Coils request
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpReadCoilsReq(const ModbusReadCoilsReq *request,
   size_t length)
{
   //Malformed request PDU?
   if(length < sizeof(ModbusReadCoilsReq))
      return ERROR_INVALID_LENGTH;

   //Dump request PDU
   TRACE_DEBUG("  Starting Address = %" PRIu16 "\r\n", ntohs(request->startingAddr));
   TRACE_DEBUG("  Quantity of Coils = %" PRIu16 "\r\n", ntohs(request->quantityOfCoils));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Read Coils response
 * @param[in] response Pointer to the response PDU
 * @param[in] length Length of the response PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpReadCoilsResp(const ModbusReadCoilsResp *response,
   size_t length)
{
   //Malformed response PDU?
   if(length < sizeof(ModbusReadCoilsResp))
      return ERROR_INVALID_LENGTH;

   //Calculate the length of the data field
   length -= sizeof(ModbusReadCoilsResp);

   //Dump response PDU
   TRACE_DEBUG("  Byte Count = %" PRIu16 "\r\n", response->byteCount);
   TRACE_DEBUG_ARRAY("  Coil Status = ", response->coilStatus, length);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Read Discrete Inputs request
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpReadDiscreteInputsReq(const ModbusReadDiscreteInputsReq *request,
   size_t length)
{
   //Malformed request PDU?
   if(length < sizeof(ModbusReadDiscreteInputsReq))
      return ERROR_INVALID_LENGTH;

   //Dump request PDU
   TRACE_DEBUG("  Starting Address = %" PRIu16 "\r\n", ntohs(request->startingAddr));
   TRACE_DEBUG("  Quantity of Inputs = %" PRIu16 "\r\n", ntohs(request->quantityOfInputs));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Read Discrete Inputs response
 * @param[in] response Pointer to the response PDU
 * @param[in] length Length of the response PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpReadDiscreteInputsResp(const ModbusReadDiscreteInputsResp *response,
   size_t length)
{
   //Malformed response PDU?
   if(length < sizeof(ModbusReadDiscreteInputsResp))
      return ERROR_INVALID_LENGTH;

   //Calculate the length of the data field
   length -= sizeof(ModbusReadDiscreteInputsResp);

   //Dump response PDU
   TRACE_DEBUG("  Byte Count = %" PRIu16 "\r\n", response->byteCount);
   TRACE_DEBUG_ARRAY("  Input Status = ", response->inputStatus, length);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Read Holding Registers request
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpReadHoldingRegsReq(const ModbusReadHoldingRegsReq *request,
   size_t length)
{
   //Malformed request PDU?
   if(length < sizeof(ModbusReadHoldingRegsReq))
      return ERROR_INVALID_LENGTH;

   //Dump request PDU
   TRACE_DEBUG("  Starting Address = %" PRIu16 "\r\n", ntohs(request->startingAddr));
   TRACE_DEBUG("  Quantity of Registers = %" PRIu16 "\r\n", ntohs(request->quantityOfRegs));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Read Holding Registers response
 * @param[in] response Pointer to the response PDU
 * @param[in] length Length of the response PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpReadHoldingRegsResp(const ModbusReadHoldingRegsResp *response,
   size_t length)
{
   //Malformed response PDU?
   if(length < sizeof(ModbusReadHoldingRegsResp))
      return ERROR_INVALID_LENGTH;

   //Calculate the length of the data field
   length -= sizeof(ModbusReadHoldingRegsResp);

   //Dump response PDU
   TRACE_DEBUG("  Byte Count = %" PRIu16 "\r\n", response->byteCount);
   TRACE_DEBUG_ARRAY("  Register Value = ", (void *) response->regValue, length);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Read Input Registers request
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpReadInputRegsReq(const ModbusReadInputRegsReq *request,
   size_t length)
{
   //Malformed request PDU?
   if(length < sizeof(ModbusReadInputRegsReq))
      return ERROR_INVALID_LENGTH;

   //Dump request PDU
   TRACE_DEBUG("  Starting Address = %" PRIu16 "\r\n", ntohs(request->startingAddr));
   TRACE_DEBUG("  Quantity of Registers = %" PRIu16 "\r\n", ntohs(request->quantityOfRegs));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Read Input Registers response
 * @param[in] response Pointer to the response PDU
 * @param[in] length Length of the response PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpReadInputRegsResp(const ModbusReadInputRegsResp *response,
   size_t length)
{
   //Malformed response PDU?
   if(length < sizeof(ModbusReadInputRegsResp))
      return ERROR_INVALID_LENGTH;

   //Calculate the length of the data field
   length -= sizeof(ModbusReadInputRegsResp);

   //Dump response PDU
   TRACE_DEBUG("  Byte Count = %" PRIu16 "\r\n", response->byteCount);
   TRACE_DEBUG_ARRAY("  Register Value = ", (void *) response->regValue, length);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Write Single Coil request
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpWriteSingleCoilReq(const ModbusWriteSingleCoilReq *request,
   size_t length)
{
   //Malformed request PDU?
   if(length < sizeof(ModbusWriteSingleCoilReq))
      return ERROR_INVALID_LENGTH;

   //Dump request PDU
   TRACE_DEBUG("  Output Address = %" PRIu16 "\r\n", ntohs(request->outputAddr));
   TRACE_DEBUG("  Output Value = %" PRIu16 "\r\n", ntohs(request->outputValue));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Write Single Coil response
 * @param[in] response Pointer to the response PDU
 * @param[in] length Length of the response PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpWriteSingleCoilResp(const ModbusWriteSingleCoilResp *response,
   size_t length)
{
   //Malformed response PDU?
   if(length < sizeof(ModbusWriteSingleCoilResp))
      return ERROR_INVALID_LENGTH;

   //Dump response PDU
   TRACE_DEBUG("  Output Address = %" PRIu16 "\r\n", ntohs(response->outputAddr));
   TRACE_DEBUG("  Output Value = %" PRIu16 "\r\n", ntohs(response->outputValue));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Write Single Register request
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpWriteSingleRegReq(const ModbusWriteSingleRegReq *request,
   size_t length)
{
   //Malformed request PDU?
   if(length < sizeof(ModbusWriteSingleRegReq))
      return ERROR_INVALID_LENGTH;

   //Dump request PDU
   TRACE_DEBUG("  Register Address = %" PRIu16 "\r\n", ntohs(request->regAddr));
   TRACE_DEBUG("  Register Value = %" PRIu16 "\r\n", ntohs(request->regValue));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Write Single Register response
 * @param[in] response Pointer to the response PDU
 * @param[in] length Length of the response PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpWriteSingleRegResp(const ModbusWriteSingleRegResp *response,
   size_t length)
{
   //Malformed response PDU?
   if(length < sizeof(ModbusWriteSingleRegResp))
      return ERROR_INVALID_LENGTH;

   //Dump response PDU
   TRACE_DEBUG("  Register Address = %" PRIu16 "\r\n", ntohs(response->regAddr));
   TRACE_DEBUG("  Register Value = %" PRIu16 "\r\n", ntohs(response->regValue));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Write Multiple Coils request
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpWriteMultipleCoilsReq(const ModbusWriteMultipleCoilsReq *request,
   size_t length)
{
   //Malformed request PDU?
   if(length < sizeof(ModbusWriteMultipleCoilsReq))
      return ERROR_INVALID_LENGTH;

   //Calculate the length of the data field
   length -= sizeof(ModbusWriteMultipleCoilsReq);

   //Dump request PDU
   TRACE_DEBUG("  Starting Address = %" PRIu16 "\r\n", ntohs(request->startingAddr));
   TRACE_DEBUG("  Quantity of Outputs = %" PRIu16 "\r\n", ntohs(request->quantityOfOutputs));
   TRACE_DEBUG("  Byte Count = %" PRIu16 "\r\n", request->byteCount);
   TRACE_DEBUG_ARRAY("  Output Value = ", request->outputValue, length);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Write Multiple Coils response
 * @param[in] response Pointer to the response PDU
 * @param[in] length Length of the response PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpWriteMultipleCoilsResp(const ModbusWriteMultipleCoilsResp *response,
   size_t length)
{
   //Malformed response PDU?
   if(length < sizeof(ModbusWriteMultipleCoilsResp))
      return ERROR_INVALID_LENGTH;

   //Dump response PDU
   TRACE_DEBUG("  Starting Address = %" PRIu16 "\r\n", ntohs(response->startingAddr));
   TRACE_DEBUG("  Quantity of Outputs = %" PRIu16 "\r\n", ntohs(response->quantityOfOutputs));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Write Multiple Registers request
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpWriteMultipleRegsReq(const ModbusWriteMultipleRegsReq *request,
   size_t length)
{
   //Malformed request PDU?
   if(length < sizeof(ModbusWriteMultipleRegsReq))
      return ERROR_INVALID_LENGTH;

   //Calculate the length of the data field
   length -= sizeof(ModbusWriteMultipleRegsReq);

   //Dump request PDU
   TRACE_DEBUG("  Starting Address = %" PRIu16 "\r\n", ntohs(request->startingAddr));
   TRACE_DEBUG("  Quantity of Registers = %" PRIu16 "\r\n", ntohs(request->quantityOfRegs));
   TRACE_DEBUG("  Byte Count = %" PRIu16 "\r\n", request->byteCount);
   TRACE_DEBUG_ARRAY("  Register Value = ", (void *) request->regValue, length);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Write Multiple Registers response
 * @param[in] response Pointer to the response PDU
 * @param[in] length Length of the response PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpWriteMultipleRegsResp(const ModbusWriteMultipleRegsResp *response,
   size_t length)
{
   //Malformed response PDU?
   if(length < sizeof(ModbusWriteMultipleRegsResp))
      return ERROR_INVALID_LENGTH;

   //Dump response PDU
   TRACE_DEBUG("  Starting Address = %" PRIu16 "\r\n", ntohs(response->startingAddr));
   TRACE_DEBUG("  Quantity of Registers = %" PRIu16 "\r\n", ntohs(response->quantityOfRegs));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Mask Write Register request
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpMaskWriteRegReq(const ModbusMaskWriteRegReq *request,
   size_t length)
{
   //Malformed request PDU?
   if(length < sizeof(ModbusMaskWriteRegReq))
      return ERROR_INVALID_LENGTH;

   //Dump request PDU
   TRACE_DEBUG("  Reference Address = %" PRIu16 "\r\n", ntohs(request->referenceAddr));
   TRACE_DEBUG("  And Value = %" PRIu16 "\r\n", ntohs(request->andMask));
   TRACE_DEBUG("  Or Value = %" PRIu16 "\r\n", ntohs(request->orMask));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Mask Write Register response
 * @param[in] response Pointer to the response PDU
 * @param[in] length Length of the response PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpMaskWriteRegResp(const ModbusMaskWriteRegResp *response,
   size_t length)
{
   //Malformed response PDU?
   if(length < sizeof(ModbusMaskWriteRegResp))
      return ERROR_INVALID_LENGTH;

   //Dump response PDU
   TRACE_DEBUG("  Reference Address = %" PRIu16 "\r\n", ntohs(response->referenceAddr));
   TRACE_DEBUG("  And Value = %" PRIu16 "\r\n", ntohs(response->andMask));
   TRACE_DEBUG("  Or Value = %" PRIu16 "\r\n", ntohs(response->orMask));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Read Write Multiple Registers request
 * @param[in] request Pointer to the request PDU
 * @param[in] length Length of the request PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpReadWriteMultipleRegsReq(const ModbusReadWriteMultipleRegsReq *request,
   size_t length)
{
   //Malformed request PDU?
   if(length < sizeof(ModbusReadWriteMultipleRegsReq))
      return ERROR_INVALID_LENGTH;

   //Calculate the length of the data field
   length -= sizeof(ModbusReadWriteMultipleRegsReq);

   //Dump request PDU
   TRACE_DEBUG("  Read Starting Address = %" PRIu16 "\r\n", ntohs(request->readStartingAddr));
   TRACE_DEBUG("  Quantity to Read = %" PRIu16 "\r\n", ntohs(request->quantityToRead));
   TRACE_DEBUG("  Write Starting Address = %" PRIu16 "\r\n", ntohs(request->writeStartingAddr));
   TRACE_DEBUG("  Quantity to Write = %" PRIu16 "\r\n", ntohs(request->quantityToWrite));
   TRACE_DEBUG("  Write Byte Count = %" PRIu16 "\r\n", request->writeByteCount);
   TRACE_DEBUG_ARRAY("  Write Register Value = ", (void *) request->writeRegValue, length);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Read Write Multiple Registers response
 * @param[in] response Pointer to the response PDU
 * @param[in] length Length of the response PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpReadWriteMultipleRegsResp(const ModbusReadWriteMultipleRegsResp *response,
   size_t length)
{
   //Malformed response PDU?
   if(length < sizeof(ModbusReadWriteMultipleRegsResp))
      return ERROR_INVALID_LENGTH;

   //Calculate the length of the data field
   length -= sizeof(ModbusReadWriteMultipleRegsResp);

   //Dump response PDU
   TRACE_DEBUG("  Read Byte Count = %" PRIu16 "\r\n", response->readByteCount);
   TRACE_DEBUG_ARRAY("  Read Register Value = ", (void *) response->readRegValue, length);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump Exception response
 * @param[in] response Pointer to the response PDU
 * @param[in] length Length of the response PDU, in bytes
 * @return Error code
 **/

error_t modbusDumpExceptionResp(const ModbusExceptionResp *response,
   size_t length)
{
#if (MODBUS_TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   const char_t *label;

   //Malformed response PDU?
   if(length < sizeof(ModbusExceptionResp))
      return ERROR_INVALID_LENGTH;

   //Get the description of the exception code
   if(response->exceptionCode < arraysize(modbusExceptionCodeLabel))
      label = modbusExceptionCodeLabel[response->exceptionCode];
   else
      label = "";

   //Dump response PDU
   TRACE_DEBUG("  Function Code = %" PRIu8 " (Exception)\r\n", response->functionCode);
   TRACE_DEBUG("  Exception Code = %" PRIu16 " (%s)\r\n", response->exceptionCode, label);
#endif

   //Successful processing
   return NO_ERROR;
}

#endif
