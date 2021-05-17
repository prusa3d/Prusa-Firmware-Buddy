/**
 * @file modbus_client_misc.c
 * @brief Helper functions for Modbus/TCP client
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
#include "modbus/modbus_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MODBUS_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Perform Modbus transaction
 * @param[in] context Pointer to the Modbus/TCP client context
 * @return Error code
 **/

error_t modbusClientTransaction(ModbusClientContext *context)
{
   error_t error;
   size_t n;
   systime_t time;
   uint8_t *pdu;

   //Initialize status code
   error = NO_ERROR;

   //Get current time
   time = osGetSystemTime();

   //Adjust timeout
   if(timeCompare(context->timestamp + context->timeout, time) > 0)
   {
      socketSetTimeout(context->socket, context->timestamp +
         context->timeout - time);
   }
   else
   {
      socketSetTimeout(context->socket, 0);
   }

   //Check current state
   if(context->state == MODBUS_CLIENT_STATE_SENDING)
   {
      //Send Modbus request
      if(context->requestAduPos < context->requestAduLen)
      {
         //Send more data
         error = modbusClientSendData(context,
            context->requestAdu + context->requestAduPos,
            context->requestAduLen - context->requestAduPos,
            &n, SOCKET_FLAG_NO_DELAY);

         //Check status code
         if(error == NO_ERROR || error == ERROR_TIMEOUT)
         {
            //Advance data pointer
            context->requestAduPos += n;
         }
      }
      else
      {
         //Flush receive buffer
         context->responseAduLen = 0;
         context->responseAduPos = 0;

         //Wait for response ADU
         context->state = MODBUS_CLIENT_STATE_RECEIVING;
      }
   }
   else if(context->state == MODBUS_CLIENT_STATE_RECEIVING)
   {
      //Receive Modbus response
      if(context->responseAduPos < sizeof(ModbusHeader))
      {
         //Receive more data
         error = modbusClientReceiveData(context,
            context->responseAdu + context->responseAduPos,
            sizeof(ModbusHeader) - context->responseAduPos, &n, 0);

         //Check status code
         if(error == NO_ERROR)
         {
            //Advance data pointer
            context->responseAduPos += n;

            //MBAP header successfully received?
            if(context->responseAduPos >= sizeof(ModbusHeader))
            {
               //Parse MBAP header
               error = modbusClientParseMbapHeader(context);
            }
         }
      }
      else if(context->responseAduPos < context->responseAduLen)
      {
         //Receive more data
         error = modbusClientReceiveData(context,
            context->responseAdu + context->responseAduPos,
            context->responseAduLen - context->responseAduPos, &n, 0);

         //Check status code
         if(error == NO_ERROR)
         {
            //Advance data pointer
            context->responseAduPos += n;
         }
      }
      else
      {
         //Point to the Modbus response PDU
         pdu = modbusClientGetResponsePdu(context, &n);

         //Debug message
         TRACE_INFO("Modbus Client: Response PDU received (%" PRIuSIZE " bytes)...\r\n", n);
         //Dump the contents of the PDU for debugging purpose
         modbusDumpResponsePdu(pdu, n);

         //Check whether the received response matches the request
         error = modbusClientCheckResp(context);

         //Check status code
         if(error == NO_ERROR)
         {
            //If the transaction identifier refers to a pending transaction,
            //the response must be parsed in order to send a confirmation to
            //the user application
            context->state = MODBUS_CLIENT_STATE_COMPLETE;
         }
         else if(error == ERROR_WRONG_IDENTIFIER)
         {
            //If the transaction identifier does not refer to any pending
            //transaction, the response must be discarded
            context->responseAduLen = 0;
            context->responseAduPos = 0;

            //Catch exception
            error = NO_ERROR;
         }
         else
         {
            //A protocol error has occurred
         }
      }
   }
   else
   {
      //Report an error
      error = ERROR_WRONG_STATE;
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = modbusClientCheckTimeout(context);
   }

   //Modbus transaction failed?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Revert to default state
      context->state = MODBUS_CLIENT_STATE_CONNECTED;
   }

   //Return status code
   return error;
}


/**
 * @brief Check whether the received response matches the request
 * @param[in] context Pointer to the Modbus/TCP client context
 * @return Error code
 **/

error_t modbusClientCheckResp(ModbusClientContext *context)
{
   error_t error;
   ModbusHeader *requestHeader;
   ModbusHeader *responseHeader;

   //Malformed request?
   if(context->requestAduLen < (sizeof(ModbusHeader) + sizeof(uint8_t)))
      return ERROR_INVALID_LENGTH;

   //Malformed response?
   if(context->responseAduLen < (sizeof(ModbusHeader) + sizeof(uint8_t)))
      return ERROR_INVALID_LENGTH;

   //Point to the MBAP header of the Modbus request
   requestHeader = (ModbusHeader *) context->requestAdu;
   //Point to the MBAP header of the Modbus response
   responseHeader = (ModbusHeader *) context->responseAdu;

   //Check transaction identifier
   if(responseHeader->transactionId != requestHeader->transactionId)
      return ERROR_WRONG_IDENTIFIER;

   //Check unit identifier
   if(responseHeader->unitId != requestHeader->unitId)
      return ERROR_UNEXPECTED_RESPONSE;

   //Check function code
   if((responseHeader->pdu[0] & MODBUS_FUNCTION_CODE_MASK) !=
      (requestHeader->pdu[0] & MODBUS_FUNCTION_CODE_MASK))
   {
      return ERROR_UNEXPECTED_RESPONSE;
   }

   //Exception response?
   if((responseHeader->pdu[0] & MODBUS_EXCEPTION_MASK) != 0)
   {
      //If the server receives the request without a communication error,
      //but cannot handle it, the server will return an exception response
      //informing the client of the nature of the error
      error = modbusClientParseExceptionResp(context);
   }
   else
   {
      //A normal response has been received
      error = NO_ERROR;
   }

   //Return status code
   return error;
}


/**
 * @brief Format MBAP header
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[in] length Length of the PDU, in bytes
 * @return Error code
 **/

error_t modbusClientFormatMbapHeader(ModbusClientContext *context,
   size_t length)
{
   ModbusHeader *header;

   //Point to the beginning of the request ADU
   header = (ModbusHeader *) context->requestAdu;

   //The transaction identifier is used to uniquely identify the matching
   //requests and responses
   context->transactionId++;

   //Format MBAP header
   header->transactionId = htons(context->transactionId);
   header->protocolId = HTONS(MODBUS_PROTOCOL_ID);
   header->length = htons(length + sizeof(uint8_t));
   header->unitId = context->unitId;

   //Compute the length of the request ADU
   context->requestAduLen = length + sizeof(ModbusHeader);

   //Debug message
   TRACE_DEBUG("Modbus Client: Sending ADU (%" PRIuSIZE " bytes)...\r\n",
      context->requestAduLen);

   //Dump MBAP header
   TRACE_DEBUG("  Transaction ID = %" PRIu16 "\r\n", ntohs(header->transactionId));
   TRACE_DEBUG("  Protocol ID = %" PRIu16 "\r\n", ntohs(header->protocolId));
   TRACE_DEBUG("  Length = %" PRIu16 "\r\n", ntohs(header->length));
   TRACE_DEBUG("  Unit ID = %" PRIu16 "\r\n", header->unitId);

   //Rewind to the beginning of the transmit buffer
   context->requestAduPos = 0;
   //Save current time
   context->timestamp = osGetSystemTime();
   //Send the request ADU to the server
   context->state = MODBUS_CLIENT_STATE_SENDING;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse MBAP header
 * @param[in] context Pointer to the Modbus/TCP client context
 * @return Error code
 **/

error_t modbusClientParseMbapHeader(ModbusClientContext *context)
{
   size_t n;
   ModbusHeader *header;

   //Sanity check
   if(context->responseAduPos < sizeof(ModbusHeader))
      return ERROR_INVALID_LENGTH;

   //Point to the beginning of the response ADU
   header = (ModbusHeader *) context->responseAdu;

   //The length field is a byte count of the following fields, including the
   //unit identifier and data fields
   n = ntohs(header->length);

   //Malformed Modbus response?
   if(n < sizeof(uint8_t))
      return ERROR_INVALID_LENGTH;

   //Retrieve the length of the PDU
   n -= sizeof(uint8_t);

   //Debug message
   TRACE_DEBUG("Modbus Client: ADU received (%" PRIuSIZE " bytes)...\r\n",
      sizeof(ModbusHeader) + n);

   //Dump MBAP header
   TRACE_DEBUG("  Transaction ID = %" PRIu16 "\r\n", ntohs(header->transactionId));
   TRACE_DEBUG("  Protocol ID = %" PRIu16 "\r\n", ntohs(header->protocolId));
   TRACE_DEBUG("  Length = %" PRIu16 "\r\n", ntohs(header->length));
   TRACE_DEBUG("  Unit ID = %" PRIu16 "\r\n", header->unitId);

   //Check protocol identifier
   if(ntohs(header->protocolId) != MODBUS_PROTOCOL_ID)
      return ERROR_WRONG_IDENTIFIER;

   //The length of the Modbus PDU is limited to 253 bytes
   if(n > MODBUS_MAX_PDU_SIZE)
      return ERROR_INVALID_LENGTH;

   //Compute the length of the response ADU
   context->responseAduLen = sizeof(ModbusHeader) + n;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Retrieve request PDU
 * @param[in] context Pointer to the Modbus/TCP client context
 * @return Pointer to the request PDU
 **/

void *modbusClientGetRequestPdu(ModbusClientContext *context)
{
   //Point to the request PDU
   return context->requestAdu + sizeof(ModbusHeader);
}


/**
 * @brief Retrieve response PDU
 * @param[in] context Pointer to the Modbus/TCP client context
 * @param[out] length Length of the response PDU, in bytes
 * @return Pointer to the response PDU
 **/

void *modbusClientGetResponsePdu(ModbusClientContext *context, size_t *length)
{
   uint8_t *responsePdu;

   //Point to the response PDU
   responsePdu = context->responseAdu + sizeof(ModbusHeader);

   //Retrieve the length of the PDU
   if(context->responseAduLen >= sizeof(ModbusHeader))
      *length = context->responseAduLen - sizeof(ModbusHeader);
   else
      *length = 0;

   //Return a pointer to the response PDU
   return responsePdu;
}


/**
 * @brief Determine whether a timeout error has occurred
 * @param[in] context Pointer to the Modbus/TCP client context
 * @return Error code
 **/

error_t modbusClientCheckTimeout(ModbusClientContext *context)
{
#if (NET_RTOS_SUPPORT == DISABLED)
   error_t error;
   systime_t time;

   //Get current time
   time = osGetSystemTime();

   //Check whether the timeout has elapsed
   if(timeCompare(time, context->timestamp + context->timeout) >= 0)
   {
      //Report a timeout error
      error = ERROR_TIMEOUT;
   }
   else
   {
      //The operation would block
      error = ERROR_WOULD_BLOCK;
   }

   //Return status code
   return error;
#else
   //Report a timeout error
   return ERROR_TIMEOUT;
#endif
}

#endif
