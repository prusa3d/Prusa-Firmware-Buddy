/**
 * @file http_client_misc.c
 * @brief Helper functions for HTTP client
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
#define TRACE_LEVEL HTTP_TRACE_LEVEL

//Dependencies
#include <limits.h>
#include <stdlib.h>
#include "core/net.h"
#include "http/http_client.h"
#include "http/http_client_auth.h"
#include "http/http_client_transport.h"
#include "http/http_client_misc.h"
#include "str.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (HTTP_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Update HTTP client state
 * @param[in] context Pointer to the HTTP client context
 * @param[in] newState New state to switch to
 **/

void httpClientChangeState(HttpClientContext *context,
   HttpClientState newState)
{
   //Update HTTP connection state
   context->state = newState;

   //Save current time
   context->timestamp = osGetSystemTime();
}


/**
 * @brief Update HTTP request state
 * @param[in] context Pointer to the HTTP client context
 * @param[in] newState New state to switch to
 **/

void httpClientChangeRequestState(HttpClientContext *context,
   HttpRequestState newState)
{
   //Update HTTP request state
   context->requestState = newState;

   //Save current time
   context->timestamp = osGetSystemTime();
}


/**
 * @brief Format default HTTP request header
 * @param[in] context Pointer to the HTTP client context
 * @return Error code
 **/

error_t httpClientFormatRequestHeader(HttpClientContext *context)
{
   size_t n;
   const char_t *version;

   //Check HTTP version
   if(context->version == HTTP_VERSION_1_0)
   {
      //Select protocol version 1.0
      version = "HTTP/1.0";

      //Under HTTP/1.0, connections are not considered persistent unless a
      //keepalive header is included
      context->keepAlive = FALSE;

      //a persistent connection with an HTTP/1.0 client cannot make use of
      //the chunked transfer-coding, and therefore must use a Content-Length
      //for marking the ending boundary of each message (refer to RFC 2068,
      //section 19.7.1)
      context->chunkedEncoding = FALSE;
   }
   else if(context->version == HTTP_VERSION_1_1)
   {
      //Select protocol version 1.1
      version = "HTTP/1.1";

      //In HTTP/1.1, all connections are considered persistent unless declared
      //otherwise
      context->keepAlive = TRUE;

      //The chunked encoding modifies the body of a message in order to transfer
      //it as a series of chunks, each with its own size indicator
      context->chunkedEncoding = FALSE;
   }
   else
   {
      //Unknown HTTP version
      return ERROR_INVALID_VERSION;
   }

   //Set default HTTP request method
   osStrcpy(context->method, "GET");

   //The Request-Line begins with a method token, followed by the Request-URI
   //and the protocol version, and ending with CRLF
   n = osSprintf(context->buffer, "GET / %s\r\n", version);

   //Terminate the HTTP request header with a CRLF sequence
   n += osSprintf(context->buffer + n, "\r\n");

   //Set the length of the request header
   context->bufferLen = n;
   context->bufferPos = 0;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format chunk-size field
 * @param[in] context Pointer to the HTTP client context
 * @param[in] length Size of the chunk
 * @return Error code
 **/

error_t httpClientFormatChunkSize(HttpClientContext *context, size_t length)
{
   size_t n = 0;

   //The chunked encoding modifies the body in order to transfer it as a
   //series of chunks, each with its own size indicator
   context->bodyLen = length;
   context->bodyPos = 0;

   //Check whether the chunk is the first one or not
   if(context->requestState == HTTP_REQ_STATE_SEND_CHUNK_DATA)
   {
      //Terminate the data of the previous chunk with a CRLF sequence
      n += osSprintf(context->buffer + n, "\r\n");
   }

   //The chunk-size field is a string of hex digits indicating the size of the
   //chunk
   n += osSprintf(context->buffer + n, "%" PRIXSIZE "\r\n", length);

   //Check whether the chunk is the last one
   if(length == 0)
   {
      //The trailer allows the sender to include additional HTTP header fields
      //at the end of the message. The trailer is terminated with a CRLF
      n += osSprintf(context->buffer + n, "\r\n");
   }

   //Set the length of the chunk-size field
   context->bufferLen = n;
   context->bufferPos = 0;

   //The chunked encoding is ended by any chunk whose size is zero
   if(length == 0)
   {
      //The last chunk is followed by an optional trailer
      httpClientChangeRequestState(context, HTTP_REQ_STATE_FORMAT_TRAILER);
   }
   else
   {
      //Send the chunk-size field
      httpClientChangeRequestState(context, HTTP_REQ_STATE_SEND_CHUNK_SIZE);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse HTTP status line
 * @param[in] context Pointer to the HTTP client context
 * @param[in] line Pointer to the status line
 * @param[in] length Length of the status line
 * @return Error code
 **/

error_t httpClientParseStatusLine(HttpClientContext *context, char_t *line,
   size_t length)
{
   error_t error;
   char_t *p;
   char_t *token;

   //Properly terminate the string with a NULL character
   line[length] = '\0';

   //Debug message
   TRACE_DEBUG("HTTP response header:\r\n%s\r\n", line);

   //The string must contains visible characters only
   error = httpCheckCharset(line, length, HTTP_CHARSET_TEXT);
   //Any error to report?
   if(error)
      return error;

   //Parse HTTP-Version field
   token = osStrtok_r(line, " ", &p);
   //Any parsing error?
   if(token == NULL)
      return ERROR_INVALID_SYNTAX;

   //Check protocol version
   if(!osStrcasecmp(token, "HTTP/1.0"))
   {
      //Under HTTP/1.0, connections are not considered persistent unless a
      //keepalive header is included
      context->keepAlive = FALSE;
   }
   else if(!osStrcasecmp(token, "HTTP/1.1"))
   {
      //In HTTP/1.1, all connections are considered persistent unless declared
      //otherwise
      if(context->version != HTTP_VERSION_1_1)
         context->keepAlive = FALSE;
   }
   else
   {
      //Unknown HTTP version
      return ERROR_INVALID_VERSION;
   }

   //Parse Status-Code field
   token = osStrtok_r(NULL, " ", &p);
   //Any parsing error?
   if(token == NULL)
      return ERROR_INVALID_SYNTAX;

   //The Status-Code element is a 3-digit integer
   context->statusCode = osStrtoul(token, &p, 10);
   //Any parsing error?
   if(*p != '\0')
      return ERROR_INVALID_SYNTAX;

   //The chunked encoding modifies the body of a message in order to transfer
   //it as a series of chunks, each with its own size indicator
   context->chunkedEncoding = FALSE;

   //For response message without a declared message body length, the message
   //body length is determined by the number of octets received prior to the
   //server closing the connection
   context->bodyLen = UINT_MAX;

   //Flush receive buffer
   context->bufferLen = 0;
   context->bufferPos = 0;
   context->bodyPos = 0;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse HTTP response header field
 * @param[in] context Pointer to the HTTP client context
 * @param[in] line Pointer to the header field
 * @param[in] length Length of the header field
 * @return Error code
 **/

error_t httpClientParseHeaderField(HttpClientContext *context, char_t *line,
   size_t length)
{
   error_t error;
   char_t *name;
   size_t nameLen;
   char_t *value;
   size_t valueLen;
   char_t *separator;

   //Properly terminate the string with a NULL character
   line[length] = '\0';

   //Debug message
   TRACE_DEBUG("%s\r\n", line);

   //The string must contains visible characters only
   error = httpCheckCharset(line, length, HTTP_CHARSET_TEXT);
   //Any error to report?
   if(error)
      return error;

   //Header field values can be folded onto multiple lines if the continuation
   //line begins with a space or horizontal tab
   if(line[0] == ' ' || line[0] == '\t')
   {
      //A continuation line cannot immediately follows the Status-Line
      if(context->bufferPos == 0)
         return ERROR_INVALID_SYNTAX;

      //Remove optional leading and trailing whitespace
      value = strTrimWhitespace(line);
      //Retrieve the length of the resulting string
      valueLen = osStrlen(value);

      //Sanity check
      if(valueLen > 0)
      {
         //The folding LWS is replaced with a single SP before interpretation
         //of the TEXT value (refer to RFC 2616, section 2.2)
         context->buffer[context->bufferPos - 1] = ' ';

         //Save field value
         osMemmove(context->buffer + context->bufferPos, value, valueLen + 1);

         //Update the size of the hash table
         context->bufferLen = context->bufferPos + valueLen + 1;
      }
   }
   else
   {
      //Each header field consists of a case-insensitive field name followed
      //by a colon, optional leading whitespace, the field value, and optional
      //trailing whitespace (refer to RFC 7230, section 3.2)
      separator = osStrchr(line, ':');

      //Any parsing error?
      if(separator == NULL)
         return ERROR_INVALID_SYNTAX;

      //Split the line
      *separator = '\0';

      //Remove optional leading and trailing whitespace
      name = strTrimWhitespace(line);
      value = strTrimWhitespace(separator + 1);

      //Retrieve the length of the resulting strings
      nameLen = osStrlen(name);
      valueLen = osStrlen(value);

      //The field name cannot be empty
      if(nameLen == 0)
         return ERROR_INVALID_SYNTAX;

      //Check header field name
      if(!osStrcasecmp(name, "Connection"))
      {
         //Parse Connection header field
         httpClientParseConnectionField(context, value);
      }
      else if(!osStrcasecmp(name, "Transfer-Encoding"))
      {
         //Parse Transfer-Encoding header field
         httpClientParseTransferEncodingField(context, value);
      }
      else if(!osStrcasecmp(name, "Content-Length"))
      {
         //Parse Content-Length header field
         httpClientParseContentLengthField(context, value);
      }
#if (HTTP_CLIENT_AUTH_SUPPORT == ENABLED)
      //WWW-Authenticate header field found?
      else if(!osStrcasecmp(name, "WWW-Authenticate"))
      {
         //Parse WWW-Authenticate header field
         httpClientParseWwwAuthenticateField(context, value);
      }
#endif
      else
      {
         //Discard unknown header fields
      }

      //Save each header field into a hash table by field name
      osMemmove(context->buffer + context->bufferPos, name, nameLen + 1);

      //Save field value
      osMemmove(context->buffer + context->bufferPos + nameLen + 1, value,
         valueLen + 1);

      //Update the size of the hash table
      context->bufferLen = context->bufferPos + nameLen + valueLen + 2;
   }

   //Decode the next header field
   context->bufferPos = context->bufferLen;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Connection header field
 * @param[in] context Pointer to the HTTP client context
 * @param[in] value NULL-terminated string that contains the field value
 * @return Error code
 **/

error_t httpClientParseConnectionField(HttpClientContext *context,
   const char_t *value)
{
   size_t n;

   //Parse the comma-separated list
   while(value[0] != '\0')
   {
      //Get the length of the current token
      n = strcspn(value, ", \t");

      //Check current value
      if(n == 10 && !osStrncasecmp(value, "keep-alive", 10))
      {
         //Check HTTP request state
         if(context->requestState == HTTP_REQ_STATE_FORMAT_HEADER)
         {
            //Make the connection persistent
            context->keepAlive = TRUE;
         }
      }
      else if(n == 5 && !osStrncasecmp(value, "close", 5))
      {
         //The connection will be closed after completion of the response
         context->keepAlive = FALSE;
      }
      else
      {
         //Just for sanity
      }

      //Advance the pointer over the separator
      if(value[n] != '\0')
         n++;

      //Point to the next token
      value += n;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Transfer-Encoding header field
 * @param[in] context Pointer to the HTTP client context
 * @param[in] value NULL-terminated string that contains the field value
 * @return Error code
 **/

error_t httpClientParseTransferEncodingField(HttpClientContext *context,
   const char_t *value)
{
   //Check the value of the header field
   if(!osStrcasecmp(value, "chunked"))
   {
      //If a Transfer-Encoding header field is present and the chunked
      //transfer coding is the final encoding, the message body length
      //is determined by reading and decoding the chunked data until the
      //transfer coding indicates the data is complete
      context->chunkedEncoding = TRUE;

      //If a message is received with both a Transfer-Encoding and a
      //Content-Length header field, the Transfer-Encoding overrides
      //the Content-Length (refer to RFC 7230, section 3.3.3)
      context->bodyLen = 0;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Content-Length header field
 * @param[in] context Pointer to the HTTP client context
 * @param[in] value NULL-terminated string that contains the field value
 * @return Error code
 **/

error_t httpClientParseContentLengthField(HttpClientContext *context,
   const char_t *value)
{
   char_t *p;

   //If a valid Content-Length header field is present without
   //Transfer-Encoding, its decimal value defines the expected message
   //body length in octets (refer to RFC 7230, section 3.3.3)
   if(!context->chunkedEncoding)
   {
      //Retrieve the length of the body
      context->bodyLen = osStrtoul(value, &p, 10);

      //Any parsing error?
      if(*p != '\0')
         return ERROR_INVALID_SYNTAX;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse chunk-size field
 * @param[in] context Pointer to the HTTP client context
 * @param[in] line Pointer to the chunk-size field
 * @param[in] length Length of the chunk-size field
 * @return Error code
 **/

error_t httpClientParseChunkSize(HttpClientContext *context, char_t *line,
   size_t length)
{
   char_t *p;

   //Properly terminate the string with a NULL character
   line[length] = '\0';

   //Remove leading and trailing whitespace
   line = strTrimWhitespace(line);

   //The chunk-size field is a string of hex digits indicating the size
   //of the chunk
   context->bodyLen = osStrtoul(line, &p, 16);

   //Any parsing error?
   if(*p != '\0')
      return ERROR_INVALID_SYNTAX;

   //Flush receive buffer
   context->bufferLen = 0;
   context->bufferPos = 0;
   context->bodyPos = 0;

   //The chunked encoding is ended by any chunk whose size is zero
   if(context->bodyLen == 0)
   {
      //The last chunk is followed by an optional trailer
      httpClientChangeRequestState(context, HTTP_REQ_STATE_RECEIVE_TRAILER);
   }
   else
   {
      //Receive chunk data
      httpClientChangeRequestState(context, HTTP_REQ_STATE_RECEIVE_CHUNK_DATA);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Determine whether a timeout error has occurred
 * @param[in] context Pointer to the HTTP client context
 * @return Error code
 **/

error_t httpClientCheckTimeout(HttpClientContext *context)
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
