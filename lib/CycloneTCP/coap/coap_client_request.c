/**
 * @file coap_client_request.c
 * @brief CoAP request handling
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
#define TRACE_LEVEL COAP_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include "core/net.h"
#include "coap/coap_client.h"
#include "coap/coap_client_request.h"
#include "coap/coap_client_block.h"
#include "coap/coap_client_misc.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (COAP_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Initialize a new CoAP request
 * @param[in] context Pointer to the CoAP client context
 * @return Pointer to the new CoAP request
 **/

CoapClientRequest *coapClientCreateRequest(CoapClientContext *context)
{
   uint_t i;
   CoapClientRequest *request;
   CoapMessageHeader *header;

   //Initialize request handle
   request = NULL;

   //Make sure the CoAP client context is valid
   if(context != NULL)
   {
      //Acquire exclusive access to the CoAP client context
      osAcquireMutex(&context->mutex);

      //Loop through the CoAP request table
      for(i = 0; i < COAP_CLIENT_NSTART; i++)
      {
         //Unused request found?
         if(context->request[i].state == COAP_REQ_STATE_UNUSED)
         {
            //Point to the current request
            request = &context->request[i];

            //Initialize request state
            request->state = COAP_REQ_STATE_INIT;
            //Attach CoAP client context
            request->context = context;
            //Set default request timeout
            request->timeout = context->timeout;
            //Initialize request callback
            request->callback = NULL;

#if (COAP_CLIENT_BLOCK_SUPPORT == ENABLED)
            //Default TX block size
            request->txBlockSzx = coapClientGetMaxBlockSize();
            //Default RX block size
            request->rxBlockSzx = COAP_BLOCK_SIZE_RESERVED;
#endif
            //Point to the CoAP message header
            header = (CoapMessageHeader *) request->message.buffer;

            //Set default parameters
            header->version = COAP_VERSION_1;
            header->type = COAP_TYPE_CON;
            header->tokenLen = (uint8_t) context->tokenLen;
            header->code = COAP_CODE_GET;
            header->mid = 0;

            //A Token is used to match responses to requests independently from
            //the underlying message
            coapClientGenerateToken(context, header);

            //Calculate the length of the CoAP message
            request->message.length = sizeof(CoapMessageHeader) + header->tokenLen;

            //We are done
            break;
         }
      }

      //Release exclusive access to the CoAP client context
      osReleaseMutex(&context->mutex);
   }

   //Return a handle to the freshly created request
   return request;
}


/**
 * @brief Set request timeout
 * @param[in] request CoAP request handle
 * @param[in] timeout Timeout value, in milliseconds
 * @return Error code
 **/

error_t coapClientSetRequestTimeout(CoapClientRequest *request,
   systime_t timeout)
{
   //Make sure the CoAP request handle is valid
   if(request == NULL)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the CoAP client context
   osAcquireMutex(&request->context->mutex);
   //Save timeout value
   request->timeout = timeout;
   //Release exclusive access to the CoAP client context
   osReleaseMutex(&request->context->mutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Send a CoAP request
 * @param[in] request CoAP request handle
 * @param[in] callback Callback function to invoke when the request completes
 * @param[in] param Callback function parameter
 * @return Error code
 **/

error_t coapClientSendRequest(CoapClientRequest *request,
   CoapRequestCallback callback, void *param)
{
   error_t error;
   CoapClientContext *context;
   CoapMessageHeader *header;

   //Make sure the CoAP request handle is valid
   if(request == NULL)
      return ERROR_INVALID_PARAMETER;

   //Point to the CoAP client context
   context = request->context;

   //Acquire exclusive access to the CoAP client context
   osAcquireMutex(&context->mutex);

   //Initialize status code
   error = NO_ERROR;

   //Send the CoAP request and wait for the response
   while(!error)
   {
      //Check current state
      if(request->state == COAP_REQ_STATE_INIT)
      {
         //Point to the CoAP message header
         header = (CoapMessageHeader *) request->message.buffer;

         //The message ID is a 16-bit unsigned integer that is generated by
         //the sender of a Confirmable or Non-confirmable message
         coapClientGenerateMessageId(context, header);

         //Reset retransmission counter
         request->retransmitCount = 0;

         //Save callback function
         request->callback = callback;
         request->param = param;

         //Send CoAP request
         error = coapClientChangeRequestState(request, COAP_REQ_STATE_TRANSMIT);
      }
      else
      {
         //Any callback function defined?
         if(request->callback != NULL)
         {
            //The CoAP client operates in asynchronous mode when a callback
            //function has been defined
            break;
         }
         else
         {
            //When the CoAP client operates in synchronous mode, the function
            //blocks until a response is received or a timeout occurs
            if(request->state == COAP_REQ_STATE_TRANSMIT ||
               request->state == COAP_REQ_STATE_RECEIVE ||
               request->state == COAP_REQ_STATE_SEPARATE)
            {
               //Wait for a response to be received
               error = coapClientProcessEvents(context,
                  COAP_CLIENT_TICK_INTERVAL);

#if (NET_RTOS_SUPPORT == DISABLED)
               //Check status code
               if(error == NO_ERROR)
               {
                  //The CoAP client operates in non-blocking mode
                  if(request->state == COAP_REQ_STATE_TRANSMIT ||
                     request->state == COAP_REQ_STATE_RECEIVE ||
                     request->state == COAP_REQ_STATE_SEPARATE)
                  {
                     //Exit immediately
                     error = ERROR_WOULD_BLOCK;
                  }
              }
#endif
            }
            else if(request->state == COAP_REQ_STATE_OBSERVE ||
               request->state == COAP_REQ_STATE_DONE)
            {
               //A valid response has been received
               break;
            }
            else if(request->state == COAP_REQ_STATE_RESET)
            {
               //A Reset message has been received
               error = ERROR_FAILURE;
            }
            else if(request->state == COAP_REQ_STATE_TIMEOUT)
            {
               //Report a timeout error
               error = ERROR_TIMEOUT;
            }
            else
            {
               //Invalid state
               error = ERROR_WRONG_STATE;
            }
         }
      }
   }

   //Release exclusive access to the CoAP client context
   osReleaseMutex(&context->mutex);

   //Return status code
   return error;
}


/**
 * @brief Cancel an outstanding CoAP request
 * @param[in] request CoAP request handle
 * @return Pointer to the CoAP request
 **/

error_t coapClientCancelRequest(CoapClientRequest *request)
{
   error_t error;
#if (COAP_CLIENT_OBSERVE_SUPPORT == ENABLED)
   uint32_t value;
   CoapMessageHeader *header;
#endif

   //Make sure the CoAP request handle is valid
   if(request == NULL)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the CoAP client context
   osAcquireMutex(&request->context->mutex);

#if (COAP_CLIENT_OBSERVE_SUPPORT == ENABLED)
   //Search the CoAP request for an Observe option
   error = coapGetUintOption(&request->message, COAP_OPT_OBSERVE, 0, &value);

   //Check whether the option has been found
   if(!error)
   {
      //Point to the CoAP message header
      header = (CoapMessageHeader *) request->message.buffer;

      //The message ID is a 16-bit unsigned integer that is generated by
      //the sender of a Confirmable or Non-confirmable message
      coapClientGenerateMessageId(request->context, header);

      //Set the Observe option to the value 1 (deregister)
      error = coapSetUintOption(&request->message, COAP_OPT_OBSERVE, 0,
         COAP_OBSERVE_DEREGISTER);

      //Check status code
      if(!error)
      {
         //Reset retransmission counter
         request->retransmitCount = 0;

         //A client may explicitly deregister by issuing a GET request that has
         //the Token field set to the token of the observation to be cancelled
         //and includes an Observe Option with the value set to 1 (deregister)
         error = coapClientChangeRequestState(request, COAP_REQ_STATE_TRANSMIT);
      }
      else
      {
         //Cancel the specified request
         error = coapClientChangeRequestState(request, COAP_REQ_STATE_CANCELED);
         //Release the resources associated with the CoAP request
         request->state = COAP_REQ_STATE_UNUSED;
      }
   }
   else
#endif
   {
      //Cancel the specified request
      error = coapClientChangeRequestState(request, COAP_REQ_STATE_CANCELED);
      //Release the resources associated with the CoAP request
      request->state = COAP_REQ_STATE_UNUSED;
   }

   //Release exclusive access to the CoAP client context
   osReleaseMutex(&request->context->mutex);

   //Return status code
   return error;
}


/**
 * @brief Release the resources associated with a CoAP request
 * @param[in] request CoAP request handle
 **/

void coapClientDeleteRequest(CoapClientRequest *request)
{
   //Valid CoAP request?
   if(request != NULL)
   {
      //Acquire exclusive access to the CoAP client context
      osAcquireMutex(&request->context->mutex);
      //The request is no more in use
      request->state = COAP_REQ_STATE_UNUSED;
      //Release exclusive access to the CoAP client context
      osReleaseMutex(&request->context->mutex);
   }
}


/**
 * @brief Get request message
 * @param[in] request CoAP request handle
 * @return Pointer to the request message
 **/

CoapMessage *coapClientGetRequestMessage(CoapClientRequest *request)
{
   CoapClientContext *context;
   CoapMessage *message;

   //Point to the CoAP client context
   context = request->context;

   //Acquire exclusive access to the CoAP client context
   osAcquireMutex(&context->mutex);

   //Check request state
   if(request->state == COAP_REQ_STATE_INIT ||
      request->state == COAP_REQ_STATE_OBSERVE ||
      request->state == COAP_REQ_STATE_DONE ||
      request->state == COAP_REQ_STATE_RESET ||
      request->state == COAP_REQ_STATE_TIMEOUT ||
      request->state == COAP_REQ_STATE_CANCELED)
   {
      //Re-initialize request state
      coapClientChangeRequestState(request, COAP_REQ_STATE_INIT);
      //Point to the request message
      message = &request->message;
   }
   else
   {
      //The request cannot be accessed while the message exchange is on-going
      message = NULL;
   }

   //Release exclusive access to the CoAP client context
   osReleaseMutex(&context->mutex);

   //Return a pointer to the request message
   return message;
}


/**
 * @brief Get response message
 * @param[in] request CoAP request handle
 * @return Pointer to the response message
 **/

CoapMessage *coapClientGetResponseMessage(CoapClientRequest *request)
{
   CoapClientContext *context;
   CoapMessage *message;

   //Point to the CoAP client context
   context = request->context;

   //Acquire exclusive access to the CoAP client context
   osAcquireMutex(&context->mutex);

   //Check request state
   if(request->state == COAP_REQ_STATE_INIT ||
      request->state == COAP_REQ_STATE_OBSERVE ||
      request->state == COAP_REQ_STATE_DONE)
   {
      //Point to the response message
      message = &context->response;
   }
   else
   {
      //The response cannot be accessed while the message exchange is on-going
      message = NULL;
   }

   //Release exclusive access to the CoAP client context
   osReleaseMutex(&context->mutex);

   //Return a pointer to the response message
   return message;
}


/**
 * @brief Set message type
 * @param[in] message Pointer to the CoAP message
 * @param[in] type Message type (Confirmable or Non-confirmable)
 * @return Error code
 **/

error_t coapClientSetType(CoapMessage *message, CoapMessageType type)
{
   //Make sure the CoAP message is valid
   if(message == NULL)
      return ERROR_INVALID_PARAMETER;

   //Set message type
   return coapSetType(message, type);
}


/**
 * @brief Get message type
 * @param[in] message Pointer to the CoAP message
 * @param[out] type Message type
 * @return Error code
 **/

error_t coapClientGetType(const CoapMessage *message, CoapMessageType *type)
{
   //Check parameters
   if(message == NULL || type == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get message type
   return coapGetType(message, type);
}


/**
 * @brief Set request method
 * @param[in] message Pointer to the CoAP message
 * @param[in] code Method code (GET, POST, PUT or DELETE)
 * @return Error code
 **/

error_t coapClientSetMethodCode(CoapMessage *message, CoapCode code)
{
   //Make sure the CoAP message is valid
   if(message == NULL)
      return ERROR_INVALID_PARAMETER;

   //Set request method
   return coapSetCode(message, code);
}


/**
 * @brief Get request method
 * @param[in] message Pointer to the CoAP message
 * @param[out] code Method code
 * @return Error code
 **/

error_t coapClientGetMethodCode(const CoapMessage *message, CoapCode *code)
{
   //Check parameters
   if(message == NULL || code == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get request method
   return coapGetCode(message, code);
}


/**
 * @brief Get response code
 * @param[in] message Pointer to the CoAP message
 * @param[out] code Response code
 * @return Error code
 **/

error_t coapClientGetResponseCode(const CoapMessage *message, CoapCode *code)
{
   //Check parameters
   if(message == NULL || code == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get response code
   return coapGetCode(message, code);
}


/**
 * @brief Set Uri-Path option
 * @param[in] message Pointer to the CoAP message
 * @param[in] path NULL-terminated string that contains the path component
 * @return Error code
 **/

error_t coapClientSetUriPath(CoapMessage *message, const char_t *path)
{
   //Check parameters
   if(message == NULL || path == NULL)
      return ERROR_INVALID_PARAMETER;

   //Encode the path component into multiple Uri-Path options
   return coapSplitRepeatableOption(message, COAP_OPT_URI_PATH, path, '/');
}


/**
 * @brief Get Uri-Path option
 * @param[in] message Pointer to the CoAP message
 * @param[out] path Pointer to the buffer where to copy the path component
 * @param[in] maxLen Maximum number of characters the buffer can hold
 * @return Error code
 **/

error_t coapClientGetUriPath(const CoapMessage *message, char_t *path,
   size_t maxLen)
{
   //Check parameters
   if(message == NULL || path == NULL)
      return ERROR_INVALID_PARAMETER;

   //Reconstruct the path component from Uri-Path options
   return coapJoinRepeatableOption(message, COAP_OPT_URI_PATH, path,
      maxLen, '/');
}


/**
 * @brief Set Uri-Query option
 * @param[in] message Pointer to the CoAP message
 * @param[in] queryString NULL-terminated string that contains the query string
 * @return Error code
 **/

error_t coapClientSetUriQuery(CoapMessage *message, const char_t *queryString)
{
   //Check parameters
   if(message == NULL || queryString == NULL)
      return ERROR_INVALID_PARAMETER;

   //Encode the query string into multiple Uri-Query options
   return coapSplitRepeatableOption(message, COAP_OPT_URI_QUERY,
      queryString, '&');
}


/**
 * @brief Get Uri-Query option
 * @param[in] message Pointer to the CoAP message
 * @param[out] queryString Pointer to the buffer where to copy the query string
 * @param[in] maxLen Maximum number of characters the buffer can hold
 * @return Error code
 **/

error_t coapClientGetUriQuery(const CoapMessage *message, char_t *queryString,
   size_t maxLen)
{
   //Check parameters
   if(message == NULL || queryString == NULL)
      return ERROR_INVALID_PARAMETER;

   //Reconstruct the query string from Uri-Query options
   return coapJoinRepeatableOption(message, COAP_OPT_URI_QUERY, queryString,
      maxLen, '&');
}


/**
 * @brief Get Location-Path option
 * @param[in] message Pointer to the CoAP message
 * @param[out] path Pointer to the buffer where to copy the path component
 * @param[in] maxLen Maximum number of characters the buffer can hold
 * @return Error code
 **/

error_t coapClientGetLocationPath(const CoapMessage *message, char_t *path,
   size_t maxLen)
{
   //Check parameters
   if(message == NULL || path == NULL)
      return ERROR_INVALID_PARAMETER;

   //Reconstruct the path component from Location-Path options
   return coapJoinRepeatableOption(message, COAP_OPT_LOCATION_PATH, path,
      maxLen, '/');
}


/**
 * @brief Get Location-Query option
 * @param[in] message Pointer to the CoAP message
 * @param[out] queryString Pointer to the buffer where to copy the query string
 * @param[in] maxLen Maximum number of characters the buffer can hold
 * @return Error code
 **/

error_t coapClientGetLocationQuery(const CoapMessage *message,
   char_t *queryString, size_t maxLen)
{
   //Check parameters
   if(message == NULL || queryString == NULL)
      return ERROR_INVALID_PARAMETER;

   //Reconstruct the query string from Location-Query options
   return coapJoinRepeatableOption(message, COAP_OPT_LOCATION_QUERY,
      queryString, maxLen, '&');
}


/**
 * @brief Add an opaque option to the CoAP message
 * @param[in] message Pointer to the CoAP message
 * @param[in] optionNum Option number
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[in] optionValue Pointer to the first byte of the option value
 * @param[in] optionLen Length of the option, in bytes
 * @return Error code
 **/

error_t coapClientSetOpaqueOption(CoapMessage *message, uint16_t optionNum,
   uint_t optionIndex, const uint8_t *optionValue, size_t optionLen)
{
   //Make sure the CoAP message is valid
   if(message == NULL)
      return ERROR_INVALID_PARAMETER;

   //Inconsistent option value?
   if(optionValue == NULL && optionLen != 0)
      return ERROR_INVALID_PARAMETER;

   //Add the specified option to the CoAP message
   return coapSetOption(message, optionNum, optionIndex, optionValue,
      optionLen);
}


/**
 * @brief Add a string option to the CoAP message
 * @param[in] message Pointer to the CoAP message
 * @param[in] optionNum Option number
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[in] optionValue NULL-terminated string that contains the option value
 * @return Error code
 **/

error_t coapClientSetStringOption(CoapMessage *message, uint16_t optionNum,
   uint_t optionIndex, const char_t *optionValue)
{
   size_t n;

   //Check parameters
   if(message == NULL || optionValue == NULL)
      return ERROR_INVALID_PARAMETER;

   //Retrieve the length of the string
   n = osStrlen(optionValue);

   //Add the specified option to the CoAP message
   return coapSetOption(message, optionNum, optionIndex,
      (const uint8_t *) optionValue, n);
}


/**
 * @brief Add a uint option to the CoAP message
 * @param[in] message Pointer to the CoAP message
 * @param[in] optionNum Option number
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[in] optionValue Option value (unsigned integer)
 * @return Error code
 **/

error_t coapClientSetUintOption(CoapMessage *message, uint16_t optionNum,
   uint_t optionIndex, uint32_t optionValue)
{
   //Make sure the CoAP message is valid
   if(message == NULL)
      return ERROR_INVALID_PARAMETER;

   //Add the specified option to the CoAP message
   return coapSetUintOption(message, optionNum, optionIndex, optionValue);
}


/**
 * @brief Read an opaque option from the CoAP message
 * @param[in] message Pointer to the CoAP message
 * @param[in] optionNum Option number to search for
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[out] optionValue Pointer to the first byte of the option value
 * @param[out] optionLen Length of the option, in bytes
 * @return Error code
 **/

error_t coapClientGetOpaqueOption(const CoapMessage *message, uint16_t optionNum,
   uint_t optionIndex, const uint8_t **optionValue, size_t *optionLen)
{
   //Check parameters
   if(message == NULL || optionValue == NULL || optionLen == NULL)
      return ERROR_INVALID_PARAMETER;

   //Search the CoAP message for the specified option number
   return coapGetOption(message, optionNum, optionIndex, optionValue,
      optionLen);
}


/**
 * @brief Read a string option from the CoAP message
 * @param[in] message Pointer to the CoAP message
 * @param[in] optionNum Option number to search for
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[out] optionValue Pointer to the first byte of the option value
 * @param[out] optionLen Length of the option, in characters
 * @return Error code
 **/

error_t coapClientGetStringOption(const CoapMessage *message, uint16_t optionNum,
   uint_t optionIndex, const char_t **optionValue, size_t *optionLen)
{
   //Check parameters
   if(message == NULL || optionValue == NULL || optionLen == NULL)
      return ERROR_INVALID_PARAMETER;

   //Search the CoAP message for the specified option number
   return coapGetOption(message, optionNum, optionIndex,
      (const uint8_t **) optionValue, optionLen);
}


/**
 * @brief Read an uint option from the CoAP message
 * @param[in] message Pointer to the CoAP message
 * @param[in] optionNum Option number to search for
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[out] optionValue Option value (unsigned integer)
 * @return Error code
 **/

error_t coapClientGetUintOption(const CoapMessage *message, uint16_t optionNum,
   uint_t optionIndex, uint32_t *optionValue)
{
   //Check parameters
   if(message == NULL || optionValue == NULL)
      return ERROR_INVALID_PARAMETER;

   //Search the CoAP message for the specified option number
   return coapGetUintOption(message, optionNum, optionIndex, optionValue);
}


/**
 * @brief Remove an option from the CoAP message
 * @param[in] message Pointer to the CoAP message
 * @param[in] optionNum Option number
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @return Error code
 **/

error_t coapClientDeleteOption(CoapMessage *message, uint16_t optionNum,
   uint_t optionIndex)
{
   //Make sure the CoAP message is valid
   if(message == NULL)
      return ERROR_INVALID_PARAMETER;

   //Remove the specified option from the CoAP message
   return coapDeleteOption(message, optionNum, optionIndex);
}


/**
 * @brief Set message payload
 * @param[in] message Pointer to the CoAP message
 * @param[out] payload Pointer to request payload
 * @param[out] payloadLen Length of the payload, in bytes
 * @return Error code
 **/

error_t coapClientSetPayload(CoapMessage *message, const void *payload,
   size_t payloadLen)
{
   //Make sure the CoAP message is valid
   if(message == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check parameters
   if(payload == NULL && payloadLen != 0)
      return ERROR_INVALID_PARAMETER;

   //Set message payload
   return coapSetPayload(message, payload, payloadLen);
}


/**
 * @brief Get message payload
 * @param[in] message Pointer to the CoAP message
 * @param[out] payload Pointer to the first byte of the payload
 * @param[out] payloadLen Length of the payload, in bytes
 * @return Error code
 **/

error_t coapClientGetPayload(const CoapMessage *message, const uint8_t **payload,
   size_t *payloadLen)
{
   //Check parameters
   if(message == NULL || payload == NULL || payloadLen == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get response payload
   return coapGetPayload(message, payload, payloadLen);
}


/**
 * @brief Write payload data
 * @param[in] message Pointer to the CoAP message
 * @param[in] data Pointer to a buffer containing the data to be written
 * @param[in] length Number of bytes to written
 * @return Error code
 **/

error_t coapClientWritePayload(CoapMessage *message, const void *data,
   size_t length)
{
   //Check parameters
   if(message == NULL || data == NULL)
      return ERROR_INVALID_PARAMETER;

   //Write payload data
   return coapWritePayload(message, data, length);
}


/**
 * @brief Read payload data
 * @param[in] message Pointer to the CoAP message
 * @param[out] data Buffer into which received data will be placed
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] length Number of bytes that have been received
 * @return Error code
 **/

error_t coapClientReadPayload(CoapMessage *message, void *data, size_t size,
   size_t *length)
{
   //Check parameters
   if(message == NULL || data == NULL)
      return ERROR_INVALID_PARAMETER;

   //Read payload data
   return coapReadPayload(message, data, size, length);
}

#endif
