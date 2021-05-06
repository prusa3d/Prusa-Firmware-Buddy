/**
 * @file coap_message.c
 * @brief CoAP message formatting and parsing
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
#include "core/net.h"
#include "coap/coap_client.h"
//#include "coap/coap_server.h"
#include "coap/coap_message.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (COAP_CLIENT_SUPPORT == ENABLED || COAP_SERVER_SUPPORT == ENABLED)


/**
 * @brief Parse CoAP message
 * @param[in] message Pointer to the CoAP message
 * @return Error code
 **/

error_t coapParseMessage(const CoapMessage *message)
{
   error_t error;
   size_t n;
   size_t length;
   const uint8_t *p;

   //Point to the first byte of the CoAP message
   p = message->buffer;
   //Retrieve the length of the message
   length = message->length;

   //Parse message header
   error = coapParseMessageHeader(p, length, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the first option of the message
   p += n;
   //Number of bytes left to process
   length -= n;

   //Parse the list of options
   error = coapParseOptions(p, length, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the payload
   p += n;
   //Number of bytes left to process
   length -= n;

   //The payload is optional
   if(length > 0)
   {
      //The payload is prefixed by a fixed, one-byte payload marker
      p++;
      //Retrieve the length of the payload
      length--;

      //The presence of a marker followed by a zero-length payload must be
      //processed as a message format error
      if(length == 0)
         return ERROR_INVALID_MESSAGE;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse CoAP message header
 * @param[in] p Input stream where to read the CoAP message header
 * @param[in] length Number of bytes available in the input stream
 * @param[out] consumed Total number of bytes that have been consumed
 * @return Error code
 **/

error_t coapParseMessageHeader(const uint8_t *p, size_t length,
   size_t *consumed)
{
   CoapMessageHeader *header;

   //Malformed CoAP message?
   if(length < sizeof(CoapMessageHeader))
      return ERROR_INVALID_HEADER;

   //Point to the CoAP message header
   header = (CoapMessageHeader *) p;

   //Check version field
   if(header->version != COAP_VERSION_1)
      return ERROR_INVALID_VERSION;

   //The length of the Token field must 0-8 bytes
   if(header->tokenLen > COAP_MAX_TOKEN_LEN)
      return ERROR_INVALID_HEADER;

   //Malformed CoAP message?
   if(length < (sizeof(CoapMessageHeader) + header->tokenLen))
      return ERROR_INVALID_HEADER;

   //Total number of bytes that have been consumed
   *consumed = sizeof(CoapMessageHeader) + header->tokenLen;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set message type
 * @param[in] message Pointer to the CoAP message
 * @param[in] type Message type (Confirmable or Non-confirmable)
 * @return Error code
 **/

error_t coapSetType(CoapMessage *message, CoapMessageType type)
{
   CoapMessageHeader *header;

   //Malformed CoAP message?
   if(message->length < sizeof(CoapMessageHeader))
      return ERROR_INVALID_MESSAGE;

   //Point to the CoAP message header
   header = (CoapMessageHeader *) message->buffer;
   //Set message type
   header->type = type;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get message type
 * @param[in] message Pointer to the CoAP message
 * @param[out] type Message type
 * @return Error code
 **/

error_t coapGetType(const CoapMessage *message, CoapMessageType *type)
{
   CoapMessageHeader *header;

   //Malformed CoAP message?
   if(message->length < sizeof(CoapMessageHeader))
      return ERROR_INVALID_MESSAGE;

   //Point to the CoAP message header
   header = (CoapMessageHeader *) message->buffer;
   //Retrieve message type
   *type = (CoapMessageType) header->type;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set method or response code
 * @param[in] message Pointer to the CoAP message
 * @param[in] code Method or response code
 * @return Error code
 **/

error_t coapSetCode(CoapMessage *message, CoapCode code)
{
   CoapMessageHeader *header;

   //Malformed CoAP message?
   if(message->length < sizeof(CoapMessageHeader))
      return ERROR_INVALID_MESSAGE;

   //Point to the CoAP message header
   header = (CoapMessageHeader *) message->buffer;
   //Set method or response code
   header->code = code;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get method or response code
 * @param[in] message Pointer to the CoAP message
 * @param[out] code Method or response code
 * @return Error code
 **/

error_t coapGetCode(const CoapMessage *message, CoapCode *code)
{
   CoapMessageHeader *header;

   //Malformed CoAP message?
   if(message->length < sizeof(CoapMessageHeader))
      return ERROR_INVALID_MESSAGE;

   //Point to the CoAP message header
   header = (CoapMessageHeader *) message->buffer;
   //Retrieve method or response code
   *code = (CoapCode) header->code;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set CoAP message payload
 * @param[in] message Pointer to the CoAP message
 * @param[out] payload Pointer to payload data
 * @param[out] payloadLen Length of the payload, in bytes
 * @return Error code
 **/

error_t coapSetPayload(CoapMessage *message, const void *payload,
   size_t payloadLen)
{
   error_t error;
   size_t n;
   size_t length;
   uint8_t *p;

   //Point to the first byte of the CoAP message
   p = message->buffer;
   //Retrieve the length of the message
   length = message->length;

   //Parse message header
   error = coapParseMessageHeader(p, length, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the first option of the message
   p += n;
   //Number of bytes left to process
   length -= n;

   //Parse the list of options
   error = coapParseOptions(p, length, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the payload
   p += n;
   //Number of bytes left to process
   length -= n;

   //Trim the existing payload
   message->length -= length;

   //The payload is optional
   if(payloadLen > 0)
   {
      //Make sure the output buffer is large enough to hold the payload
      if((message->length + payloadLen + 1) > COAP_MAX_MSG_SIZE)
         return ERROR_BUFFER_OVERFLOW;

      //The payload is prefixed by a fixed, one-byte payload marker
      p[0] = COAP_PAYLOAD_MARKER;

      //The payload data extends from after the marker to the end of the
      //UDP datagram
      osMemcpy(p + 1, payload, payloadLen);

      //Terminate the payload with a NULL character
      p[payloadLen + 1] = '\0';

      //Adjust the length of the CoAP message
      message->length += payloadLen + 1;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get CoAP message payload
 * @param[in] message Pointer to the CoAP message
 * @param[out] payload Pointer to the first byte of the payload
 * @param[out] payloadLen Length of the payload, in bytes
 * @return Error code
 **/

error_t coapGetPayload(const CoapMessage *message, const uint8_t **payload,
   size_t *payloadLen)
{
   error_t error;
   size_t n;
   size_t length;
   const uint8_t *p;

   //Point to the first byte of the CoAP message
   p = message->buffer;
   //Retrieve the length of the message
   length = message->length;

   //Parse message header
   error = coapParseMessageHeader(p, length, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the first option of the message
   p += n;
   //Number of bytes left to process
   length -= n;

   //Parse the list of options
   error = coapParseOptions(p, length, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the payload
   p += n;
   //Number of bytes left to process
   length -= n;

   //The payload is optional
   if(length > 0)
   {
      //The payload is prefixed by a fixed, one-byte payload marker
      p++;
      //Retrieve the length of the payload
      length--;
   }

   //Point to the first byte of the payload, if any
   *payload = p;
   //Save the length of the payload
   *payloadLen = length;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Write payload data
 * @param[in] message Pointer to the CoAP message
 * @param[in] data Pointer to a buffer containing the data to be written
 * @param[in] length Number of bytes to written
 * @return Error code
 **/

error_t coapWritePayload(CoapMessage *message, const void *data,
   size_t length)
{
   error_t error;
   size_t k;
   size_t n;
   uint8_t *p;

   //Point to the first byte of the CoAP message
   p = message->buffer;
   //Retrieve the length of the message
   n = message->length;

   //Parse message header
   error = coapParseMessageHeader(p, n, &k);
   //Any error to report?
   if(error)
      return error;

   //Point to the first option of the message
   p += k;
   //Number of bytes left to process
   n -= k;

   //Parse the list of options
   error = coapParseOptions(p, n, &k);
   //Any error to report?
   if(error)
      return error;

   //Point to the payload
   p += k;
   //Number of bytes left to process
   n -= k;

   //Any data to write?
   if(length > 0)
   {
      //The absence of the payload marker denotes a zero-length payload
      if(n == 0)
      {
         //Make sure the output buffer is large enough to hold the payload
         if((message->length + length + 1) > COAP_MAX_MSG_SIZE)
            return ERROR_BUFFER_OVERFLOW;

         //The payload is prefixed by a fixed, one-byte payload marker
         p[n++] = COAP_PAYLOAD_MARKER;

         //Adjust the length of the CoAP message
         message->length++;
      }
      else
      {
         //Make sure the output buffer is large enough to hold the payload
         if((message->length + length) > COAP_MAX_MSG_SIZE)
            return ERROR_BUFFER_OVERFLOW;
      }

      //Copy data
      osMemcpy(p + n, data, length);

      //Terminate the payload with a NULL character
      p[n + length] = '\0';

      //Adjust the length of the CoAP message
      message->length += length;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Read payload data
 * @param[in] message Pointer to the CoAP message
 * @param[out] data Buffer into which received data will be placed
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] length Number of bytes that have been received
 * @return Error code
 **/

error_t coapReadPayload(CoapMessage *message, void *data, size_t size,
   size_t *length)
{
   error_t error;
   size_t k;
   size_t n;
   const uint8_t *p;

   //Point to the first byte of the CoAP message
   p = message->buffer;
   //Retrieve the length of the message
   n = message->length;

   //Parse message header
   error = coapParseMessageHeader(p, n, &k);
   //Any error to report?
   if(error)
      return error;

   //Point to the first option of the message
   p += k;
   //Number of bytes left to process
   n -= k;

   //Parse the list of options
   error = coapParseOptions(p, n, &k);
   //Any error to report?
   if(error)
      return error;

   //Point to the payload
   p += k;
   //Number of bytes left to process
   n -= k;

   //The payload is optional
   if(n > 0)
   {
      //The payload is prefixed by a fixed, one-byte payload marker
      p++;
      //Retrieve the length of the payload
      n--;
   }

   //Any data to be copied?
   if(message->pos < n)
   {
      //Limit the number of bytes to copy at a time
      n = MIN(n - message->pos, size);

      //Copy data
      osMemcpy(data, p + message->pos, n);

      //Advance current position
      message->pos += n;
       //Total number of data that have been read
      *length = n;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //No more data available
      error = ERROR_END_OF_STREAM;
   }

   //Return status code
   return error;
}


/**
 * @brief Token comparison
 * @param[in] header1 Pointer to the first CoAP message header
 * @param[in] header2 Pointer to the second CoAP message header
 * @return TRUE if the tokens match, else FALSE
 **/

bool_t coapCompareToken(const CoapMessageHeader *header1,
   const CoapMessageHeader *header2)
{
   bool_t res = FALSE;

   //Check token lengths
   if(header1->tokenLen == header2->tokenLen)
   {
      //Compare tokens
      if(!osMemcmp(header1->token, header2->token, header1->tokenLen))
      {
         //The tokens match
         res = TRUE;
      }
   }

   //Return comparison result
   return res;
}

#endif
