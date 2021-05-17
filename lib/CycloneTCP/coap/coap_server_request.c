/**
 * @file coap_server_request.c
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
#include "coap/coap_server.h"
#include "coap/coap_server_request.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (COAP_SERVER_SUPPORT == ENABLED)


/**
 * @brief Get request method
 * @param[in] context Pointer to the CoAP server context
 * @param[out] code Method code (GET, POST, PUT or DELETE)
 * @return Error code
 **/

error_t coapServerGetMethodCode(CoapServerContext *context, CoapCode *code)
{
   //Check parameters
   if(context == NULL || code == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get request method
   return coapGetCode(&context->request, code);
}


/**
 * @brief Get Uri-Path option
 * @param[in] context Pointer to the CoAP server context
 * @param[out] path Pointer to the buffer where to copy the path component
 * @param[in] maxLen Maximum number of characters the buffer can hold
 * @return Error code
 **/

error_t coapServerGetUriPath(CoapServerContext *context, char_t *path,
   size_t maxLen)
{
   //Check parameters
   if(context == NULL || path == NULL)
      return ERROR_INVALID_PARAMETER;

   //Reconstruct the path component from Uri-Path options
   return coapJoinRepeatableOption(&context->request, COAP_OPT_URI_PATH,
      path, maxLen, '/');
}


/**
 * @brief Get Uri-Query option
 * @param[in] context Pointer to the CoAP server context
 * @param[out] queryString Pointer to the buffer where to copy the query string
 * @param[in] maxLen Maximum number of characters the buffer can hold
 * @return Error code
 **/

error_t coapServerGetUriQuery(CoapServerContext *context, char_t *queryString,
   size_t maxLen)
{
   //Check parameters
   if(context == NULL || queryString == NULL)
      return ERROR_INVALID_PARAMETER;

   //Reconstruct the query string from Uri-Query options
   return coapJoinRepeatableOption(&context->request, COAP_OPT_URI_QUERY,
      queryString, maxLen, '&');
}


/**
 * @brief Read an opaque option from the CoAP request
 * @param[in] context Pointer to the CoAP server context
 * @param[in] optionNum Option number to search for
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[out] optionValue Pointer to the first byte of the option value
 * @param[out] optionLen Length of the option, in bytes
 * @return Error code
 **/

error_t coapServerGetOpaqueOption(CoapServerContext *context, uint16_t optionNum,
   uint_t optionIndex, const uint8_t **optionValue, size_t *optionLen)
{
   //Check parameters
   if(context == NULL|| optionValue == NULL || optionLen == NULL)
      return ERROR_INVALID_PARAMETER;

   //Search the CoAP message for the specified option number
   return coapGetOption(&context->request, optionNum, optionIndex, optionValue,
      optionLen);
}


/**
 * @brief Read a string option from the CoAP request
 * @param[in] context Pointer to the CoAP server context
 * @param[in] optionNum Option number to search for
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[out] optionValue Pointer to the first byte of the option value
 * @param[out] optionLen Length of the option, in characters
 * @return Error code
 **/

error_t coapServerGetStringOption(CoapServerContext *context, uint16_t optionNum,
   uint_t optionIndex, const char_t **optionValue, size_t *optionLen)
{
   //Check parameters
   if(context == NULL|| optionValue == NULL || optionLen == NULL)
      return ERROR_INVALID_PARAMETER;

   //Search the CoAP message for the specified option number
   return coapGetOption(&context->request, optionNum, optionIndex,
      (const uint8_t **) optionValue, optionLen);
}


/**
 * @brief Read an uint option from the CoAP request
 * @param[in] context Pointer to the CoAP server context
 * @param[in] optionNum Option number to search for
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[out] optionValue Option value (unsigned integer)
 * @return Error code
 **/

error_t coapServerGetUintOption(CoapServerContext *context, uint16_t optionNum,
   uint_t optionIndex, uint32_t *optionValue)
{
   //Check parameters
   if(context == NULL|| optionValue == NULL)
      return ERROR_INVALID_PARAMETER;

   //Search the CoAP message for the specified option number
   return coapGetUintOption(&context->request, optionNum, optionIndex,
      optionValue);
}


/**
 * @brief Get request payload
 * @param[in] context Pointer to the CoAP server context
 * @param[out] payload Pointer to the first byte of the payload
 * @param[out] payloadLen Length of the payload, in bytes
 * @return Error code
 **/

error_t coapServerGetPayload(CoapServerContext *context, const uint8_t **payload,
   size_t *payloadLen)
{
   //Check parameters
   if(context == NULL|| payload == NULL || payloadLen == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get response payload
   return coapGetPayload(&context->request, payload, payloadLen);
}


/**
 * @brief Read request payload data
 * @param[in] context Pointer to the CoAP server context
 * @param[out] data Buffer into which received data will be placed
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] length Number of bytes that have been received
 * @return Error code
 **/

error_t coapServerReadPayload(CoapServerContext *context, void *data, size_t size,
   size_t *length)
{
   //Check parameters
   if(context == NULL|| data == NULL)
      return ERROR_INVALID_PARAMETER;

   //Read payload data
   return coapReadPayload(&context->request, data, size, length);
}


/**
 * @brief Set response method
 * @param[in] context Pointer to the CoAP server context
 * @param[in] code Response code
 * @return Error code
 **/

error_t coapServerSetResponseCode(CoapServerContext *context, CoapCode code)
{
   //Make sure the CoAP message is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Set response code
   return coapSetCode(&context->response, code);
}


/**
 * @brief Set Location-Path option
 * @param[in] context Pointer to the CoAP server context
 * @param[in] path NULL-terminated string that contains the path component
 * @return Error code
 **/

error_t coapServerSetLocationPath(CoapServerContext *context,
   const char_t *path)
{
   //Check parameters
   if(context == NULL|| path == NULL)
      return ERROR_INVALID_PARAMETER;

   //Encode the path component into multiple Location-Path options
   return coapSplitRepeatableOption(&context->response, COAP_OPT_LOCATION_PATH,
      path, '/');
}


/**
 * @brief Set Location-Query option
 * @param[in] context Pointer to the CoAP server context
 * @param[in] queryString NULL-terminated string that contains the query string
 * @return Error code
 **/

error_t coapServerSetLocationQuery(CoapServerContext *context,
   const char_t *queryString)
{
   //Check parameters
   if(context == NULL|| queryString == NULL)
      return ERROR_INVALID_PARAMETER;

   //Encode the query string into multiple Location-Query options
   return coapSplitRepeatableOption(&context->response, COAP_OPT_LOCATION_QUERY,
      queryString, '&');
}


/**
 * @brief Add an opaque option to the CoAP response
 * @param[in] context Pointer to the CoAP server context
 * @param[in] optionNum Option number
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[in] optionValue Pointer to the first byte of the option value
 * @param[in] optionLen Length of the option, in bytes
 * @return Error code
 **/

error_t coapServerSetOpaqueOption(CoapServerContext *context, uint16_t optionNum,
   uint_t optionIndex, const uint8_t *optionValue, size_t optionLen)
{
   //Make sure the CoAP message is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Inconsistent option value?
   if(optionValue == NULL && optionLen != 0)
      return ERROR_INVALID_PARAMETER;

   //Add the specified option to the CoAP message
   return coapSetOption(&context->response, optionNum, optionIndex,
      optionValue, optionLen);
}


/**
 * @brief Add a string option to the CoAP response
 * @param[in] context Pointer to the CoAP server context
 * @param[in] optionNum Option number
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[in] optionValue NULL-terminated string that contains the option value
 * @return Error code
 **/

error_t coapServerSetStringOption(CoapServerContext *context, uint16_t optionNum,
   uint_t optionIndex, const char_t *optionValue)
{
   size_t n;

   //Check parameters
   if(context == NULL|| optionValue == NULL)
      return ERROR_INVALID_PARAMETER;

   //Retrieve the length of the string
   n = osStrlen(optionValue);

   //Add the specified option to the CoAP message
   return coapSetOption(&context->response, optionNum, optionIndex,
      (const uint8_t *) optionValue, n);
}


/**
 * @brief Add a uint option to the CoAP response
 * @param[in] context Pointer to the CoAP server context
 * @param[in] optionNum Option number
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[in] optionValue Option value (unsigned integer)
 * @return Error code
 **/

error_t coapServerSetUintOption(CoapServerContext *context, uint16_t optionNum,
   uint_t optionIndex, uint32_t optionValue)
{
   //Make sure the CoAP message is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Add the specified option to the CoAP message
   return coapSetUintOption(&context->response, optionNum, optionIndex,
      optionValue);
}


/**
 * @brief Remove an option from the CoAP response
 * @param[in] context Pointer to the CoAP server context
 * @param[in] optionNum Option number
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @return Error code
 **/

error_t coapServerDeleteOption(CoapServerContext *context, uint16_t optionNum,
   uint_t optionIndex)
{
   //Make sure the CoAP message is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Remove the specified option from the CoAP message
   return coapDeleteOption(&context->response, optionNum, optionIndex);
}


/**
 * @brief Set response payload
 * @param[in] context Pointer to the CoAP server context
 * @param[out] payload Pointer to request payload
 * @param[out] payloadLen Length of the payload, in bytes
 * @return Error code
 **/

error_t coapServerSetPayload(CoapServerContext *context, const void *payload,
   size_t payloadLen)
{
   //Make sure the CoAP message is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check parameters
   if(payload == NULL && payloadLen != 0)
      return ERROR_INVALID_PARAMETER;

   //Set message payload
   return coapSetPayload(&context->response, payload, payloadLen);
}


/**
 * @brief Write payload data
 * @param[in] context Pointer to the CoAP server context
 * @param[in] data Pointer to a buffer containing the data to be written
 * @param[in] length Number of bytes to written
 * @return Error code
 **/

error_t coapServerWritePayload(CoapServerContext *context, const void *data,
   size_t length)
{
   //Check parameters
   if(context == NULL|| data == NULL)
      return ERROR_INVALID_PARAMETER;

   //Write payload data
   return coapWritePayload(&context->response, data, length);
}

#endif
